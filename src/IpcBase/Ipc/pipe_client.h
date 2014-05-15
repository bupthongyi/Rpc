#pragma once

#include <deque>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>

#include "ipc_message.h"
#include "ipc_buffer.h"

class pipe_client : public boost::asio::windows::stream_handle 
{
public:
	explicit pipe_client(boost::asio::io_service& service)
		: boost::asio::windows::stream_handle(service)
	{
	}
	virtual ~pipe_client()
	{
		boost::system::error_code ec;
		close(ec);
	}

	boost::system::error_code connect(const std::wstring& pipeName,int timeout_msec)
	{
		std::wstring fullPipeName = L"\\\\.\\Pipe\\" + pipeName;

		boost::system::error_code ec;
		int waitCnt=timeout_msec/10;
		int cnt=0;
		do
		{
			ec=do_connect(fullPipeName);
			if (!ec)
				break;
			Sleep(10);
		}while(++cnt<waitCnt);
		return ec;
	}

	typedef boost::function<void(boost::shared_ptr<IpcBuffer>,size_t,boost::system::error_code)> read_handler_type;
	void async_read(read_handler_type msg_handler)
	{
		read(msg_handler);
	}

	typedef boost::function<void(boost::system::error_code)> write_handler_type;
	void async_write(boost::shared_ptr<Message> msg, write_handler_type h)
	{
		write(msg, h);
	}

	boost::system::error_code close(boost::system::error_code& ec)
	{
		return boost::asio::windows::stream_handle::close(ec);
	}

private:
	boost::system::error_code do_connect(const std::wstring& fullPipeName)
	{
		HANDLE pipe_handle = ::CreateFile(
			fullPipeName.c_str(), 
			GENERIC_READ|GENERIC_WRITE, 
			0,
			NULL, 
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED, 
			NULL
			);
		if (pipe_handle == INVALID_HANDLE_VALUE)
			return boost::system::error_code(::GetLastError(), boost::asio::error::get_system_category());

		DWORD mode = PIPE_READMODE_BYTE;
		if(!::SetNamedPipeHandleState(pipe_handle, &mode, NULL, NULL))
		{
			int error = GetLastError();
			::CloseHandle(pipe_handle);
			return boost::system::error_code(error, boost::asio::error::get_system_category());
		}
		boost::system::error_code ec;
		boost::asio::windows::stream_handle::assign(pipe_handle,ec);
		if (ec)
		{
			::DisconnectNamedPipe(pipe_handle);
			::CloseHandle(pipe_handle);
		}
		return ec;
	}

	void write(boost::shared_ptr<Message> msg, write_handler_type handler)
	{
		boost::asio::async_write(*this,
			boost::asio::buffer(msg->data(), msg->size()),
			boost::asio::transfer_all(),
			boost::bind(&pipe_client::handle_write,this,_1,_2,msg,handler)
			);
	}

	void handle_write(boost::system::error_code ec, size_t len, 
		boost::shared_ptr<Message> msg, write_handler_type handler)
	{
		/*if (ec)
		{
			if (handler) 
			{	
				handler(ec,0);
			}

			printf("error = %s\n", ec.message().c_str());
			return;
		}
		else
		{
			if (handler)
				handler(ec,len);
		}*/
	}

	void read(read_handler_type handler)
	{
		boost::shared_ptr<IpcBuffer> buffer(new IpcBuffer(kReadBufferSize));
		boost::asio::async_read(*this,
			boost::asio::buffer(buffer->data(), buffer->size()),
			boost::asio::transfer_at_least(1),
			boost::bind(&pipe_client::handle_read,this,_1,_2,buffer,handler)
			);
	}

	void handle_read(boost::system::error_code ec, size_t bytes_transfered, 
		boost::shared_ptr<IpcBuffer> buffer,read_handler_type handler)
	{
		if (handler)
			handler(buffer, bytes_transfered, ec);
	}

private:
	// Amount of data to read at once from the pipe.
	static const size_t kReadBufferSize = 4 * 1024;
};