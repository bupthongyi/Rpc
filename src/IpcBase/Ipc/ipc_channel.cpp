#include "ipc_channel.h"

Channel::Channel(const std::wstring &channel, int connect_time_out, int work_thread)
	: channel_name_(channel)
	, connect_time_out_(connect_time_out)
	, work_thread_(work_thread)
{
}

Channel::~Channel()
{
	Close();
}

bool Channel::Connect()
{
	printf("open ipc\n");
	work_queue_.Start(work_thread_);
	io_service_thread_.start_thread();

	ipc_.reset(new pipe_client(io_service_thread_.get_io_service()));
	boost::system::error_code ec = ipc_->connect(channel_name_, connect_time_out_);
	if (ec)
	{
		printf("ipc_ connect error %s\n", ec.message().c_str());
		boost::system::error_code error;
		ipc_->close(error);

		io_service_thread_.get_io_service().post(
			boost::bind(&Channel::HandleError,this, ipc_,ec)
			);

		return false;
	}
	else
	{
		printf("ipc_ connect ok\n");
		ipc_->async_read(boost::bind(&Channel::HandleRead,this,_1,_2,_3));
		return true;
	}
}

void Channel::Close()
{
	if (ipc_)
	{
		boost::system::error_code ec;
		ipc_->close(ec);
		ipc_->get_io_service().post(boost::bind(&Channel::HoldIpc,ipc_));
		ipc_.reset();
	}
	io_service_thread_.stop_thread();
	work_queue_.Stop();
	listener_map_.clear();
}

bool Channel::RegisterListener(Listener *listener, int id)
{
	boost::mutex::scoped_lock lck(work_mutex_);
	listener_map_[id] = listener;
	return true;
}

bool Channel::Send(boost::shared_ptr<Message> message)
{
	ipc_->async_write(message,boost::bind(&Channel::HandleWrite,this,_1));
	return true;
}

void Channel::HandleWrite(boost::system::error_code error)
{
}


void Channel::HandleRead(boost::shared_ptr<IpcBuffer> buffer, 
				size_t input_data_len, 
				boost::system::error_code error)
{
	if (error)
		return;

	if(input_data_len > 0)
	{
		DispatchInputData((char*)buffer->data(), input_data_len);
	}

	ipc_->async_read(boost::bind(&Channel::HandleRead,this,_1,_2,_3));
}

void Channel::HandleError(
			 boost::shared_ptr<pipe_client> ipc,
			 boost::system::error_code error)
{

}

bool Channel::DispatchInputData(const char* input_data, int input_data_len)
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

bool Channel::IsInternalMessage(const Message& m)
{
	return m.type() == REGISTER_MESSAGE_TYPE || m.type() == HELLO_MESSAGE_TYPE;
}

void Channel::HandleInternalMessage(const Message& msg)
{
	// ×¢²áËùÓÐlinstener
	std::map<int, Listener*>::iterator itr = listener_map_.begin();
	for(; itr != listener_map_.end(); itr++)
	{
		int id = itr->first;
		boost::shared_ptr<Message> m(new Message(id,REGISTER_MESSAGE_TYPE,Message::PRIORITY_NORMAL));
		ipc_->async_write(m, boost::bind(&Channel::HandleWrite,this,_1));
	}
}

bool Channel::OnMessageReceived(const Message& message)
{
	boost::shared_ptr<Message> m(new Message(message));
	work_queue_.Post(boost::bind(&Channel::HandleMessageReceived, this, m));
	return true;
}

void Channel::HandleMessageReceived(boost::shared_ptr<Message> message)
{
	boost::mutex::scoped_lock lck(work_mutex_);
	std::map<int, Listener*>::iterator itr = listener_map_.find(message->routing_id());
	if(itr != listener_map_.end())
	{
		Listener* listener = itr->second;
		if(listener)
		{
			listener->OnMessageReceived(*message);
		}
	}
}