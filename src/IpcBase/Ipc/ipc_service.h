#pragma once

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "TaskQueue.h"
#include "service_thread.h"
#include "pipe_acceptor.h"
#include "pipe_service.h"

#include "ipc_message.h"
#include "ipc_define.h"

class CServer : public boost::enable_shared_from_this<CServer>
{
public:
	CServer();
	~CServer();
	void SetIpc(boost::shared_ptr<pipe_service> pipe);
	boost::shared_ptr<pipe_service> pipe_;
private:
	void HandleRead(boost::shared_ptr<IpcBuffer> buffer, 
		size_t input_data_len, 
		boost::system::error_code error);

	void HandleWrite(boost::system::error_code error);

	bool DispatchInputData(const char* input_data, int input_data_len);
	bool IsInternalMessage(const Message& m);
	void HandleInternalMessage(const Message& msg);
	bool OnMessageReceived(const Message& message);

	std::string input_overflow_buf_;
	static const size_t kMaximumMessageSize = 128 * 1024 * 1024;

	static std::map<int, boost::weak_ptr<CServer>> dispatch_map;
	static boost::shared_mutex service_mutex;
};

class IpcService : public boost::enable_shared_from_this<IpcService>
{
public:
	IpcService(const std::wstring &channel);
	~IpcService();

	bool Start();
	void Stop();

private:
	void HandleAccept(boost::shared_ptr<pipe_service> pipe, boost::system::error_code ec);

	io_service_thread io_service_thread_;
	boost::shared_ptr<pipe_acceptor> pipe_acceptor_;

	std::wstring channel_name_;
};