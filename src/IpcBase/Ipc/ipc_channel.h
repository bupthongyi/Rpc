#pragma once

#include <string>
#include <boost/asio/windows/stream_handle.hpp>

#include "TaskQueue.h"
#include "service_thread.h"
#include "pipe_client.h"

#include "ipc_sender.h"
#include "ipc_listener.h"
#include "ipc_define.h"

class Channel : public Sender
{
public:
	Channel(const std::wstring& channel, int connect_time_out = 5000, int work_thread = 1);
	~Channel();

	bool Connect();
	void Close();

	bool RegisterListener(Listener *listener, int id);
	bool Send(boost::shared_ptr<Message> message);

private:

	void HandleWrite(boost::system::error_code error);

	void HandleRead(boost::shared_ptr<IpcBuffer> buffer, 
		size_t input_data_len, 
		boost::system::error_code error);

	void HandleError(
		boost::shared_ptr<pipe_client> ipc,
		boost::system::error_code error);

	static void HoldIpc(boost::shared_ptr<pipe_client> ipc){};

	bool DispatchInputData(const char* input_data, int input_data_len);
	bool IsInternalMessage(const Message& m);
	void HandleInternalMessage(const Message& msg);
	bool OnMessageReceived(const Message& message);
	void HandleMessageReceived(boost::shared_ptr<Message> message);

	io_service_thread io_service_thread_;
	boost::shared_ptr<pipe_client> ipc_;
	TaskQueue work_queue_;

	std::wstring channel_name_;
	int connect_time_out_;
	int work_thread_;

	std::string input_overflow_buf_;
	static const size_t kMaximumMessageSize = 128 * 1024 * 1024;

	std::map<int, Listener*> listener_map_;
	boost::mutex work_mutex_;
};