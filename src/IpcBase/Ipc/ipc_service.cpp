#include "ipc_service.h"

static int connect_count = 0;
static unsigned int received = 0;

static TaskQueue task_queue;

typedef boost::shared_lock<boost::shared_mutex> readLock;
typedef boost::unique_lock<boost::shared_mutex> writeLock;

std::map<int, boost::weak_ptr<CServer>> CServer::dispatch_map;
boost::shared_mutex CServer::service_mutex;

CServer::CServer()
{
	connect_count++;
}

CServer::~CServer()
{
	connect_count--;
	printf("============= connect_count = %d\n", connect_count);
}

void CServer::SetIpc(boost::shared_ptr<pipe_service>  pipe)
{
	pipe_=pipe;

	// 对于每个新连接的client，我们发送hello消息，client接收到后立即注册监听事件
	boost::shared_ptr<Message> m(new Message(MSG_ROUTING_NONE,HELLO_MESSAGE_TYPE,Message::PRIORITY_NORMAL));
	pipe_->async_write(m, boost::bind(&CServer::HandleWrite,shared_from_this(),_1));

	pipe_->async_read(boost::bind(&CServer::HandleRead,shared_from_this(),_1,_2,_3));
}

void CServer::HandleRead(boost::shared_ptr<IpcBuffer> buffer, 
						 size_t input_data_len, 
						 boost::system::error_code error)
{
	if (error)
		return;

	if(input_data_len > 0)
	{
		received++;
		if(received%1000 == 0)
		{
			printf("%u\n", received);
		}
		DispatchInputData((char*)buffer->data(), input_data_len);
	}

	pipe_->async_read(boost::bind(&CServer::HandleRead,shared_from_this(),_1,_2,_3));
}

void CServer::HandleWrite(boost::system::error_code error)
{
}

bool CServer::DispatchInputData(const char* input_data, int input_data_len)
{
	const char* p;
	const char* end;

	// Possibly combine with the overflow buffer to make a larger buffer.
	if (input_overflow_buf_.empty()) 
	{
		p = input_data;
		end = input_data + input_data_len;
	} 
	else 
	{
		if (input_overflow_buf_.size() + input_data_len > kMaximumMessageSize) 
		{
			input_overflow_buf_.clear();
			printf("IPC message is too big\n");
			return false;
		}

		input_overflow_buf_.append(input_data, input_data_len);
		p = input_overflow_buf_.data();
		end = p + input_overflow_buf_.size();
	}

	// Dispatch all complete messages in the data buffer.
	while (p < end) 
	{
		const char* message_tail = Message::FindNext(p, end);
		if (message_tail) 
		{
			int len = static_cast<int>(message_tail - p);
			Message m(p, len);

			if (IsInternalMessage(m))
				HandleInternalMessage(m);
			else
				OnMessageReceived(m);
			
			p = message_tail;
		} 
		else 
		{
			// Last message is partial.
			break;
		}
	}

	input_overflow_buf_.assign(p, end - p);
	return true;
}

bool CServer::IsInternalMessage(const Message& m)
{
	return m.type() == REGISTER_MESSAGE_TYPE || m.type() == HELLO_MESSAGE_TYPE;
}

void CServer::HandleInternalMessage(const Message& msg)
{
	// 添加到分发线程
	writeLock lock(service_mutex);
	dispatch_map[msg.routing_id()] = shared_from_this();
}

bool CServer::OnMessageReceived(const Message& message)
{
	readLock lock(service_mutex);
	std::map<int, boost::weak_ptr<CServer>>::iterator itr = dispatch_map.find(message.routing_id());
	if(itr != dispatch_map.end())
	{
		boost::weak_ptr<CServer>& weak_server = itr->second;
		boost::shared_ptr<CServer> server = weak_server.lock();
		if(server)
		{
			boost::shared_ptr<Message> m(new Message(message));
			server->pipe_->async_write(m, boost::bind(&CServer::HandleWrite,server,_1));
		}
		else
		{
			dispatch_map.erase(itr);
		}
	}

	return true;
}


IpcService::IpcService(const std::wstring &channel)
	: channel_name_(channel)
{
	pipe_acceptor_.reset(new pipe_acceptor(io_service_thread_.get_io_service()));
}

IpcService::~IpcService()
{
}

bool IpcService::Start()
{
	task_queue.Start(4);
	io_service_thread_.start_thread();
	pipe_acceptor_->open(channel_name_);

	boost::shared_ptr<pipe_service> ipc(new pipe_service(io_service_thread_.get_io_service()));
	pipe_acceptor_->async_accept(*ipc,boost::bind(&IpcService::HandleAccept,shared_from_this(),ipc,_1));
	return true;
}

void IpcService::Stop()
{
	task_queue.Stop();
	pipe_acceptor_->close();
	io_service_thread_.stop_thread();
}

void IpcService::HandleAccept(boost::shared_ptr<pipe_service> pipe,boost::system::error_code ec)
{
	boost::shared_ptr<pipe_service> ipc(new pipe_service(io_service_thread_.get_io_service()));
	if (ec) 
	{
		Sleep(1);//wait for a moment
		printf(" ==================== CPipeIpcServer::HandleAccept error\n");
		pipe_acceptor_->async_accept(*ipc,boost::bind(&IpcService::HandleAccept,shared_from_this(),ipc,_1));
	}
	else
	{
		printf(" ==================== CPipeIpcServer::HandleAccept ok\n");
		pipe_acceptor_->async_accept(*ipc,boost::bind(&IpcService::HandleAccept,shared_from_this(),ipc,_1));
		boost::shared_ptr<CServer> sp_server(new CServer);
		if (sp_server)
		{
			sp_server->SetIpc(pipe);
		}
	}
}