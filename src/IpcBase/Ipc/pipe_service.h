#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "ipc_buffer.h"
#include "ipc_message.h"

class pipe_service : public boost::asio::windows::stream_handle 
{
public:
	explicit pipe_service(boost::asio::io_service& service)
		: boost::asio::windows::stream_handle(service)
	{
	}

	virtual ~pipe_service()
	{
		boost::system::error_code ec;
		close(ec);
	}


	typedef boost::function<void(boost::shared_ptr<IpcBuffer>,size_t,boost::system::error_code)> read_handler_type;
	void async_read(read_handler_type msg_handler)
	{
		read(msg_handler);
	}

	typedef boost::function<void(boost::system::error_code,size_t)> write_handler_type;
	void async_write(boost::shared_ptr<Message> msg, write_handler_type h)
	{
		write(msg, h);
	}

	boost::system::error_code close(boost::system::error_code& ec)
	{
		if (native_handle()!=INVALID_HANDLE_VALUE)
			::DisconnectNamedPipe(native_handle());
		return boost::asio::windows::stream_handle::close(ec);
	}

private:
	void write(boost::shared_ptr<Message> msg, write_handler_type handler)
	{
		boost::asio::async_write(*this,
			boost::asio::buffer(msg->data(), msg->size()),
			boost::asio::transfer_all(),
			boost::bind(&pipe_service::handle_write,this,_1,_2,msg,handler)
			);
	}

	void handle_write(boost::system::error_code ec, size_t len, 
		boost::shared_ptr<Message> msg, write_handler_type handler)
	{
		if (handler)
			handler(ec,len);
	}

	void read(read_handler_type handler)
	{
		boost::shared_ptr<IpcBuffer> buffer(new IpcBuffer(kReadBufferSize));
		boost::asio::async_read(*this,
			boost::asio::buffer(buffer->data(), buffer->size()),
			boost::asio::transfer_at_least(1),
			boost::bind(&pipe_service::handle_read,this,_1,_2,buffer,handler)
			);
	}

	void handle_read(boost::system::error_code ec, size_t bytes_transfered, 
		boost::shared_ptr<IpcBuffer> buffer,read_handler_type handler)
	{
		if (handler)
			handler(buffer, bytes_transfered, ec);
	}

	// Amount of data to read at once from the pipe.
	static const size_t kReadBufferSize = 4 * 1024;
};