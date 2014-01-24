#pragma once


#include <string>
#include <map>
#include <vector>
#include "zmq.h"
#include "utils.h"
#include "byte_array.h"
#include "serializer.h"

namespace zmq_rpc {

// ------------------------------------------------------------
// RPC Context Singleton Pattern.
// !!!singleton is thread unsafe.
// app better to call 'init' at first.
class Context {
public:
	static void init(int io_threads = kDefaultIOThreads);
	static Context* instance();
	void* context() const 
	{
		return zmq_context_;
	}
	int increaseConnectionNumber() 
	{
		return connection_number_++;
	}
private:
	Context(int io_threads);
	~Context();
	static const int kDefaultIOThreads = 4;
	static Context* instance_;
	int connection_number_;
	void* zmq_context_;
}; // class Context

// ------------------------------------------------------------
// RPC Request and Response Interface.
class Request: public Serializable {
public:
	const std::string& method() const;
	void set_method(const std::string& method_fun);
	Request();
	// here 'virtual' is important',otherwise there is a mem leak
	virtual ~Request() {}
protected:
	std::string method_;
};

class Response: public Serializable {
public:
	const std::string& return_value() const;
	void set_return_value(const std::string& _return_value);
	Response();
	// here 'virtual' is important',otherwise there is a mem leak
	virtual ~Response() {}
protected:
	std::string return_value_;
};

// ------------------------------------------------------------
// RPC Service. service register them.
class Service {
public:
	virtual ~Service() {}
	virtual Request* allocateRequest() = 0;
	virtual Response* allocateResponse() = 0;
	// default deallocate action is just delete it.
	virtual void deallocateRequest(Request* request) {
		delete request;
	}
	virtual void deallocateResponse(Response* response) {
		delete response;
	}
	virtual void run(const Request* request, Response* response) = 0;
	const std::string& service_name() const {
		return srv_name_;
	}
	void set_service_name(const std::string& srv_name) {
		srv_name_ = srv_name;
	}
public:
	std::string srv_name_;
};

class Connection
{
public:
	Connection();
	~Connection();
	// client
	// we don't guarantee the server already started
	// if you try to ensure that, you should guarantee on app level.
	// by call a dummy method or something else.
	int connect(const char* conn_uri);
	int reconnect();
	void close();
	int call(const Request* request, Response* response, int timeout_ms);
	int send(const Request* request);
	int recv(Response* response, int timeout_ms);
	int callVector(const std::vector< const Request* >& request_vector, const std::vector< Response* >& response_vector, int timeout_ms);
	int sendVector(const std::vector< const Request* >& request_vector);
	int recvVector(const std::vector< Response* >& response_vector, int timeout_ms);

	// server
	int serve(const char* serv_uri, int callback_threads);
	void registerService(const std::string& method, Service* service);
	int rpc_thread_function(); // use internally.
	const char* uri() const { return uri_.c_str(); }
	void stop() { stop_ = true; }
	bool working() const { return working_; }
private:
	typedef std::map< std::string, Service* > Dispatcher;
	int rpc_dispatch_function(ReadableByteArray* in_bytes, WriteableByteArray* out_bytes);
	// ----------------------------------------
	// advanced usage. much more efficient.
	// send 'data/size' with pointer, call 'free' to deallocate it after.
	int sendPtrInternalFree(void* data, size_t size);
	// recv message and store it to 'msg'
	int recvInternalMessage(zmq_msg_t* msg, int timeous_ms);
	void* zmq_main_socket_;
	void* zmq_back_socket_;
	Dispatcher dispatcher_;
	int connection_number_;
	char inproc_uri_[128];
	std::string uri_;
	bool stop_;
	bool working_;
public:
	static int kserverPollTimeoutMillSeconds;
}; // class Connection


}