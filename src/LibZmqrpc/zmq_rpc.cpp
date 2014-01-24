#include <stdio.h>
#include <string>
#include <process.h>
#include "zmq_rpc.h"

namespace zmq_rpc {
int Connection::kserverPollTimeoutMillSeconds = 500;

static bool smart_check_uri(const char* uri)
{
	std::string u(uri);
	if(u.find("tcp://")!=0)
	{ // if not tcp protocol we don't check.
		return true;
	}
	u=u.substr(strlen("tcp://"));  
	// if all digits and '.', then we think it's a ip.
	bool ip=true;
	int dotcount=0;
	int i=0;
	for(i=0;u[i] && u[i]!=':';i++)
	{
		if(isdigit(u[i]) || u[i]=='.')
		{
			if(u[i]=='.')
			{
				dotcount++;
			}
			continue;
		}
		ip=false;
	}
	if(u[i]!=':')
	{ // invalid.should follow a port.
		return false;
	}  
	if(ip)
	{
		return dotcount==3; // check '.' == 3
	}  
	// otherwise we think it's a hostname.we don't check hostname.
	return true;
}


// ------------------------------------------------------------
// wrapper of zmq
static void* _zmq_init(int io_threads)
{
	return zmq_init(io_threads);
}

static void* _zmq_socket(void* context, int type) 
{
	void* socket = zmq_socket(context, type);
	return socket;
}

static void internal_free(void* data, void* /*hint*/) 
{
	// just free it.
	free(data);
}

// put ptr of 'data/size' to 'msg', and when unneed call the 'internal_free' to deallocate 'data/size'
static int _zmq_put_msg_ptr_internal_free(zmq_msg_t* msg, void* data, size_t size) 
{
	zmq_msg_close(msg); // discard old content.
	// always OK.
	zmq_msg_init_data(msg, data, size, internal_free, NULL);
	return 0;
}

static int _zmq_connect(void* socket, const char* uri) 
{
	if(!smart_check_uri(uri))
	{
		return -1;
	}
	int ret = zmq_connect(socket, uri);
	if(ret < 0) 
	{
	}
	return ret;
}


static int _zmq_bind(void* socket, const char* uri) 
{
	if(!smart_check_uri(uri))
	{
		return -1;
	}
	int ret = zmq_bind(socket, uri);
	if(ret < 0) 
	{
	}
	return ret;
}

static int _zmq_close(void* socket) 
{
	int ret = zmq_close(socket);
	if(ret < 0) 
	{
	}
	return ret;
}

static int _zmq_recv(void* socket, zmq_msg_t* msg, int flags) 
{
	int ret = zmq_recv(socket, msg, flags);
	if(ret < 0) 
	{
	}
	return ret;
}

static int _zmq_send(void* socket, zmq_msg_t* msg, int flags) 
{
	int ret = zmq_send(socket, msg, flags);
	if(ret < 0) 
	{
	}
	return ret;
}

static int _zmq_poll(zmq_pollitem_t* items, int nitems, int timeout_ms) 
{
	int ret = zmq_poll(items, nitems, timeout_ms * 1000); // timeout unit is microseconds.
	if(ret < 0) 
	{
	}
	return ret;
}

static int _zmq_getsockopt(void* socket, int option_name, void* option_value, size_t* option_len) 
{
	int ret = zmq_getsockopt(socket, option_name, option_value, option_len);
	if(ret < 0) 
	{
	}
	return ret;
}

// copy the 'data/size' to 'msg'
static int _zmq_put_msg(zmq_msg_t* msg, const void* data, size_t size) 
{
	zmq_msg_close(msg); // discard old content.
	int ret = zmq_msg_init_size(msg, size);
	if(ret < 0) 
	{
	} 
	else 
	{
		memcpy(zmq_msg_data(msg), data, size);
	}
	return ret;
}

static int _zmq_msg_close(zmq_msg_t *msg)
{
	return zmq_msg_close (msg);
}

// ------------------------------------------------------------
// wrapper of serialization and deserilization
static bool serialize_request(const Request* request, WriteableByteArray* bytes) {
	// put method at first.
	if(!serialize(request->method(), bytes)) {
		return false;
	}
	//DSTREAM_DEBUG("serialize method(%s)", request->method().c_str());
	// put object then.
	if(!request->serialize(bytes)) {
		return false;
	}
	//DSTREAM_DEBUG("serialize request OK");
	return true;
}

static bool deserialize_method(std::string* method, ReadableByteArray* bytes) {
	// fetch method at first.
	if(!deserialize(method, bytes)) {
		return false;
	}
	//DSTREAM_DEBUG("deserialize method(%s) OK", method->c_str());
	return true;
}

static bool serialize_response(const Response* response, WriteableByteArray* bytes) {
	// put return value at first.
	if(!serialize(response->return_value(), bytes)) {
		return false;
	}
	//DSTREAM_DEBUG("serialize return value(%s)", response->return_value().c_str());
	// put object then.
	if(!response->serialize(bytes)) {
		return false;
	}
	//DSTREAM_DEBUG("serialize response OK");
	return true;
}

static bool deserialize_response(Response* response, ReadableByteArray* bytes) {
	std::string return_value;
	// fetch return value at first.
	if(!deserialize(&return_value, bytes)) {
		return false;
	}
	//DSTREAM_DEBUG("deserialize return value(%s)", return_value.c_str());
	// get object then.
	if(!response->deserialize(bytes)) {
		return false;
	}
	//DSTREAM_DEBUG("deserialize response OK");
	response->set_return_value(return_value.c_str());
	return true;
}

// zmq_msg_t operation is very brittle, especially zmq_msg_init_data
// is easy to confuse user and cause memory leak or double free.
// and I just try it add as many comments as I can when I operate zmq_msg_t.

// ------------------------------------------------------------
// Context Implementation
Context* Context::instance_ = NULL;

void Context::init(int io_threads) 
{
	if(instance_ == NULL) 
	{
		instance_ = new Context(io_threads);
	}
}

Context* Context::instance() 
{
	if(instance_ == NULL) 
	{
		instance_ = new Context(kDefaultIOThreads);
	}
	return instance_;
}

Context::Context(int io_threads) 
{
	connection_number_ = 0;
	zmq_context_ = _zmq_init(io_threads);
	if(zmq_context_ == NULL) 
	{
	}
}

Context::~Context() 
{
	int ret = zmq_term(zmq_context_);
	if(ret < 0) 
	{
	}
}

// ------------------------------------------------------------
// RPC Request and Response Implementation
Request::Request(): method_("have you set method???") {
}

const std::string& Request::method() const {
	return method_;
}

void Request::set_method(const std::string& method_fun) {
	method_ = method_fun;
}

Response::Response(): return_value_("OK") {
}

const std::string& Response::return_value() const {
	return return_value_;
}

void Response::set_return_value(const std::string& _return_value) {
	return_value_ = _return_value;
}

static unsigned int __stdcall proxy_rpc_thread_function(void* arg) 
{
	Connection* conn = static_cast<Connection*>(arg);
	return conn->rpc_thread_function(); 
}

int Connection::rpc_thread_function() 
{
	void* receiver = NULL;
	int ret = 0;
	zmq_pollitem_t items[1];
	zmq_msg_t msg;
	zmq_msg_init(&msg);

	receiver = _zmq_socket(Context::instance()->context(), ZMQ_REP);
	if(receiver == NULL) 
	{
		goto fail;
	}
	if(_zmq_connect(receiver, inproc_uri_) < 0) 
	{
		goto fail;
	}

	items[0].socket = receiver;
	items[0].fd = -1;
	items[0].events = ZMQ_POLLIN;

	while(1) 
	{
		// check connection is break.
		if(stop_) 
		{
			break;
		}
		ret = _zmq_poll(items, 1, Connection::kserverPollTimeoutMillSeconds);
		if(ret < 0) 
		{
			continue;
		} 
		else if(ret == 0) 
		{ // timeout.
			continue;
		}

		if(_zmq_recv(receiver, &msg, 0) < 0) 
		{
			continue;
		}

		// handle message
		ReadableByteArray recv_bytes(static_cast<const Byte*>(zmq_msg_data(&msg)),zmq_msg_size(&msg));
		WriteableByteArray send_bytes(true); // user free.

		if(rpc_dispatch_function(&recv_bytes, &send_bytes) < 0) 
		{
			continue;
		}

		// reply.
		ByteSize data_size;
		const Byte* data = send_bytes.data(&data_size);

		_zmq_put_msg_ptr_internal_free(&msg, const_cast<Byte*>(data), data_size);
		if(_zmq_send(receiver, &msg, 0) < 0) 
		{
			zmq_msg_close(&msg); // like sendPtrInternalFree.
			continue; // modify by lanbijia@baidu.com
		}
		// we have to init right here.
		// otherwise 'recv' or we call 'close'
		// will cause double free. be CAUTIOUS!!!.
		zmq_msg_init(&msg);
	} // while(1)

fail:
	zmq_msg_close(&msg);
	if(receiver) 
	{
		zmq_close(receiver);
		receiver = NULL;
	}
	return 0;
}

// ------------------------------------------------------------
// thread and dispatch function.
int Connection::rpc_dispatch_function(ReadableByteArray* in_bytes, WriteableByteArray* out_bytes) 
{
	int ret = -1;
	Request* request = NULL;
	Response* response = NULL;
	Service* service = NULL;
	std::string method;

	// maybe there is much more request.
	// support not only one request.
	uint64_t number = 0;
	if(!deserialize(&number, in_bytes)) 
	{
		goto fail;
	}
	if(!serialize(number, out_bytes)) 
	{
		goto fail;
	}
	for(uint64_t i = 0; i < number; i++) 
	{
		//deserialize method name
		if(!deserialize_method(&method, in_bytes)) 
		{
			continue;
		}

		Connection::Dispatcher::const_iterator it = dispatcher_.find(method);
		if(it == dispatcher_.end()) 
		{
			goto fail;
		}

		service = it->second;

		request = service->allocateRequest();
		if(NULL == request || !request->deserialize(in_bytes)) 
		{
			goto fail;
		}

		response = service->allocateResponse();

		service->run(request, response);

		if(!serialize_response(response, out_bytes)) 
		{
			goto fail;
		}

		// deallocate request and response.
		if(request) 
		{
			service->deallocateRequest(request);
			request = NULL;
		}
		if(response) 
		{
			service->deallocateResponse(response);
			response = NULL;
		}
	}
	ret = 0;
fail:
	// check again.
	if(request) 
	{
		service->deallocateRequest(request);
		request = NULL;
	}
	if(response) 
	{
		service->deallocateResponse(response);
		response = NULL;
	}
	return ret;
}

Connection::Connection()
{
	zmq_main_socket_ = NULL;
	zmq_back_socket_ = NULL;
	connection_number_ = 0;
	stop_ = false;
	working_ = false;
}

Connection::~Connection() 
{
}

int Connection::serve(const char* serv_uri, int callback_threads) 
{
	int ret = 0;
	int success_thrnum = 0;
	HANDLE *pthreadHandles = new HANDLE[callback_threads];
	zmq_pollitem_t items[2];
	zmq_msg_t msg;
	zmq_msg_init(&msg);
	__int64 more;
	size_t moresz = sizeof(more);
	uri_ = serv_uri;

	connection_number_ = Context::instance()->increaseConnectionNumber();

	// bind the main socket to recv request from client
	zmq_main_socket_ = _zmq_socket(Context::instance()->context(), ZMQ_ROUTER);
	if(zmq_main_socket_ == NULL) 
	{
		ret = -1;
		goto fail;
	}
	if(_zmq_bind(zmq_main_socket_, serv_uri) < 0) 
	{
		ret = -1;
		goto fail;
	}

	// bind the backend socket to handle the request
	zmq_back_socket_ = _zmq_socket(Context::instance()->context(), ZMQ_DEALER);
	if(zmq_back_socket_ == NULL) 
	{
		ret = -1;
		goto fail;
	}
	_snprintf(inproc_uri_, sizeof(inproc_uri_), "inproc://conn#%d", connection_number_);
	if(_zmq_bind(zmq_back_socket_, inproc_uri_) < 0) 
	{
		ret = -1;
		goto fail;
	}

	// allocate threads.
	for(success_thrnum = 0; success_thrnum < callback_threads; success_thrnum++) 
	{
		pthreadHandles[success_thrnum] = (HANDLE)_beginthreadex(
															NULL, 
															0, 
															proxy_rpc_thread_function, 
															this, 
															0, 
															NULL);
		if(pthreadHandles[success_thrnum] == NULL) 
		{
			stop_ = true; // set stop flag, so the thread function can exit.
			goto fail;
		}
	}

	// epoll main socket and back socket. also detect stop flag.
	// works as zmq device.
	items[0].socket = zmq_main_socket_;
	items[0].fd = -1;
	items[0].events = ZMQ_POLLIN;
	items[1].socket = zmq_back_socket_;
	items[1].fd = -1;
	items[1].events = ZMQ_POLLIN;
	working_ = true;
	while(1) 
	{
		// check connection is break.
		if(stop_) 
		{
			break;
		}
		ret = _zmq_poll(items, 2, kserverPollTimeoutMillSeconds);
		if(ret < 0) 
		{
			goto fail;
		} 
		else if(ret == 0) 
		{
			continue;
		}

		if(items[0].revents & ZMQ_POLLIN) 
		{
			while(1) 
			{
				if(_zmq_recv(items[0].socket, &msg, 0) < 0) 
				{
					goto fail;
				}
				if(_zmq_getsockopt(items[0].socket, ZMQ_RCVMORE, &more, &moresz) < 0) 
				{
					goto fail;
				}
				if(_zmq_send(items[1].socket, &msg, more ? ZMQ_SNDMORE : 0) < 0) 
				{
					goto fail;
				}
				if(!more) 
				{
					break;
				}
			}
		}

		if(items[1].revents & ZMQ_POLLIN) 
		{
			while(1) 
			{
				if( _zmq_recv(items[1].socket, &msg, 0) < 0) 
				{
					goto fail;
				}
				if(_zmq_getsockopt(items[1].socket, ZMQ_RCVMORE, &more, &moresz) < 0) 
				{
					goto fail;
				}
				if(_zmq_send(items[0].socket, &msg, more ? ZMQ_SNDMORE : 0) < 0) 
				{
					goto fail;
				}
				if(!more) 
				{
					break;
				}
			}
		}
	} // while(1)

fail:
	_zmq_msg_close(&msg);
	for(int i = 0; i < success_thrnum; i++) 
	{
		// ignore the return value
		DWORD dwExitCode = ::WaitForSingleObject(pthreadHandles[i], INFINITE);
		if ( WAIT_OBJECT_0 == dwExitCode )
		{
			CloseHandle(pthreadHandles[i]);
		}
	}
	//close();
	return ret;
}

void Connection::registerService(const std::string& method, Service* service) 
{
	dispatcher_[method] = service;
}

int Connection::connect(const char* conn_uri) 
{
	if(zmq_main_socket_!=NULL)
	{
		return 0;
	}
	uri_ = conn_uri;
	connection_number_ = Context::instance()->increaseConnectionNumber();
	// bind the main socket to send request
	zmq_main_socket_ = _zmq_socket(Context::instance()->context(), ZMQ_REQ);
	if(zmq_main_socket_ == NULL) 
	{
		goto fail;
	}
	if(_zmq_connect(zmq_main_socket_, conn_uri) < 0) 
	{
		goto fail;
	}
	return 0;
fail:
	return -1;
}

int Connection::reconnect() 
{
	close();
	return connect(uri_.c_str());
}

void Connection::close() 
{
	if(zmq_main_socket_) 
	{
		_zmq_close(zmq_main_socket_);
		zmq_main_socket_ = NULL;
	}
	if(zmq_back_socket_) 
	{
		_zmq_close(zmq_back_socket_);
		zmq_back_socket_ = NULL;
	}
	working_ = false;
}

int Connection::call(const Request* request, Response* response, int timeout_ms) 
{
	if(send(request) < 0) 
	{
		return -1;
	}
	if(recv(response, timeout_ms) < 0) 
	{
		return -1;
	}
	return 0;
}

int Connection::send(const Request* request) 
{
	WriteableByteArray send_bytes(true);

	// put object.
	// since there may be a lot of request put together.
	// here we declare that we want send 1 request.
	if(!serialize(static_cast<uint64_t>(1), &send_bytes)) 
	{
		return -1;
	}
	if(!serialize_request(request, &send_bytes)) 
	{
		return -1;
	}

	// send once.
	ByteSize size;
	const Byte* data = send_bytes.data(&size);
	ReadableByteArray _send_bytes(data, size);
	if(sendPtrInternalFree(const_cast<Byte*>(data), size) < 0) 
	{
		return -1;
	}
	return 0;
}

int Connection::recv(Response* response, int timeout_ms) {
	zmq_msg_t msg;
	zmq_msg_init(&msg);

	if(recvInternalMessage(&msg, timeout_ms) < 0) 
	{
		zmq_msg_close(&msg);
		return -1;
	}

	// fetch object.
	ReadableByteArray _recv_bytes(reinterpret_cast<const Byte*>(zmq_msg_data(&msg)),
		zmq_msg_size(&msg));
	// recv number of response at first.
	uint64_t number = 0;
	if(!deserialize(&number, &_recv_bytes)) 
	{
		zmq_msg_close(&msg);
		return -1;
	}
	// check number
	if(number != 1) 
	{
		zmq_msg_close(&msg);
		return -1;
	}
	if(!deserialize_response(response, &_recv_bytes)) 
	{
		zmq_msg_close(&msg);
		return -1;
	}

	zmq_msg_close(&msg);
	return 0;
}

int Connection::callVector(const std::vector< const Request* >& request_vector, const std::vector< Response* >& response_vector, int timeout_ms) 
{
	if(sendVector(request_vector) < 0) 
	{
		return -1;
	}
	if(recvVector(response_vector, timeout_ms) < 0) 
	{
		return -1;
	}
	return 0;
}

int Connection::sendVector(const std::vector< const Request* >& request_vector) 
{
	WriteableByteArray send_bytes(true);

	// put object.
	// put the number of request at first.
	uint64_t number = request_vector.size();
	if(!serialize(number, &send_bytes)) 
	{
		return -1;
	}
	for(size_t i = 0; i < number; i++) 
	{
		if(!serialize_request(request_vector[i], &send_bytes)) 
		{
			return -1;
		}
	}

	// send once.
	ByteSize size;
	const Byte* data = send_bytes.data(&size);
	ReadableByteArray _send_bytes(data, size);
	if(sendPtrInternalFree(const_cast<Byte*>(data), size) < 0) 
	{
		return -1;
	}
	return 0;
}

int Connection::recvVector(const std::vector< Response* >& response_vector, int timeout_ms) 
{
	zmq_msg_t msg;
	zmq_msg_init(&msg);

	if(recvInternalMessage(&msg, timeout_ms) < 0) 
	{
		zmq_msg_close(&msg);
		return -1;
	}

	// fetch object.
	ReadableByteArray _recv_bytes(reinterpret_cast<const Byte*>(zmq_msg_data(&msg)),
		zmq_msg_size(&msg));
	// parse number of response at first.
	uint64_t number = 0;
	if(!deserialize(&number, &_recv_bytes)) 
	{
		return -1;
	}
	for(uint64_t idx = 0; idx < number; idx++) 
	{
		if(!deserialize_response(response_vector[idx], &_recv_bytes)) 
		{
			zmq_msg_close(&msg);
			return -1;
		}
	}

	zmq_msg_close(&msg);
	return 0;
}

int Connection::sendPtrInternalFree(void* data, size_t size) 
{
	zmq_msg_t msg;
	zmq_msg_init(&msg);
	// always OK. so we can close anyway.
	_zmq_put_msg_ptr_internal_free(&msg, data, size);

	// since user don't know when to free the data.
	// so if we fail right here, we have to help user to free it.
	// otherwise user are in the fucking embrassment.
	if(_zmq_send(zmq_main_socket_, &msg, 0) < 0) 
	{
		zmq_msg_close(&msg);
		return -1;
	}
	return 0;
}

int Connection::recvInternalMessage(zmq_msg_t* msg, int timeout_ms) 
{
	zmq_pollitem_t items[1];
	items[0].socket = zmq_main_socket_;
	items[0].fd = -1;
	items[0].events = ZMQ_POLLIN;

	int ret = _zmq_poll(items, 1, timeout_ms);
	if(ret < 0) 
	{
		return -1;
	} else if(ret == 0) 
	{
		return -1;
	}
	if(_zmq_recv(zmq_main_socket_, msg, 0) < 0) 
	{
		return -1;
	}
	return 0;
}


}