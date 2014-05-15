#pragma once

#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/atomic.hpp>


class pipe_acceptor
{
public:
	pipe_acceptor(boost::asio::io_service& ioService)
		: service_(ioService)
		, pipe_handle_(INVALID_HANDLE_VALUE)
		, buffer_size_(4096)
		, session_(0)
	{
	}

	virtual ~pipe_acceptor(void)
	{
		close();
	}

	void open(std::wstring const& pipeName)
	{
		++session_;
		pipe_name_ = L"\\\\.\\Pipe\\" + pipeName;
		boost::system::error_code ec;
		create_pipe(ec);
	}

	void close(void)
	{
		++session_;
		if (INVALID_HANDLE_VALUE != pipe_handle_)
		{
			::DisconnectNamedPipe(pipe_handle_);
			::CloseHandle(pipe_handle_);
			pipe_handle_ = INVALID_HANDLE_VALUE;
		}
	}

	//typedef boost::asio::windows::stream_handle stream_handle;
	template<typename Handler>
	void async_accept(boost::asio::windows::stream_handle& stream, Handler handler)
	{
		service_.post(
			boost::bind(&pipe_acceptor::do_async_accept<Handler>,this,
			boost::ref(stream),boost::make_tuple(handler))
			);
	}

private:
	template<typename Handler>
	void do_async_accept(boost::asio::windows::stream_handle& stream, boost::tuple<Handler> handler)
	{
		if (pipe_handle_==INVALID_HANDLE_VALUE)
		{
			boost::system::error_code ec;
			create_pipe(ec);
		}

		boost::asio::windows::overlapped_ptr overlapped(service_,
			boost::bind(&pipe_acceptor::handle_accept<Handler>, this
			, boost::ref(stream), handler,_1,_2,(int)session_)
			);
		BOOL success = ::ConnectNamedPipe(pipe_handle_, overlapped.get());
		DWORD const kLastError = ::GetLastError();
		bool needCallback=false;
		if (!success)
		{
			printf(" pipe server error: lasterr=%d\n",kLastError);
			switch(kLastError)
			{
			case ERROR_PIPE_CONNECTED:
				//ERROR_PIPE_CONNECTED状态时，stream不会回掉。（这是asio的bug??）
				//所以这里主动回掉。
				needCallback=true;
				break;
			case ERROR_IO_PENDING:
				break;
			default://出错了
				boost::system::error_code error(kLastError, boost::asio::error::get_system_category());
				if (INVALID_HANDLE_VALUE != pipe_handle_)
				{
					::DisconnectNamedPipe(pipe_handle_);
					::CloseHandle(pipe_handle_);
					pipe_handle_ = INVALID_HANDLE_VALUE;
				}
				overlapped.complete(error, 0);
				return;
			}
		}

		printf(" pipe server ok: lasterr=%d\n",kLastError);
		stream.assign(pipe_handle_);
		pipe_handle_=INVALID_HANDLE_VALUE;
		overlapped.release();

		if (needCallback)
			boost::get<0>(handler)(boost::system::error_code());
	}

	template<typename Handler>
	void handle_accept(boost::asio::windows::stream_handle& stream
		, boost::tuple<Handler> handler
		, boost::system::error_code const& error
		, size_t bytesTransferred
		, int session
		)
	{
		if (session!=session_)
			return;

		boost::get<0>(handler)(error);
	}

	void create_pipe( boost::system::error_code& ec )
	{
		printf("create pipe\n");
		pipe_handle_ = ::CreateNamedPipe(pipe_name_.c_str(),
										PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
										PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
										PIPE_UNLIMITED_INSTANCES,
										buffer_size_,
										buffer_size_,
										NMPWAIT_USE_DEFAULT_WAIT,
										NULL);
		if(pipe_handle_ == INVALID_HANDLE_VALUE)
		{
			printf("create pipe error\n");
			ec=boost::system::error_code(::GetLastError(), boost::system::get_system_category());
		}
	}

private:
	std::wstring                pipe_name_;
	boost::asio::io_service&    service_;
	HANDLE                      pipe_handle_;
	DWORD                       buffer_size_;
	boost::atomic_int			session_;
};