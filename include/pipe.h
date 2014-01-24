#pragma once

#include <boost/thread/thread.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_array.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/scoped_ptr.hpp>
#pragma warning(push, 0) 
#include <boost/atomic.hpp>
#pragma warning(pop)

#include <Sddl.h>
#include <string>
#include <deque>

namespace pipe_ipc{

class CLowIntegritySecurityAttributes
{
public:
	CLowIntegritySecurityAttributes():m_pAttributes(NULL)
	{

	}
	LPSECURITY_ATTRIBUTES operator & ()
	{
		if( NULL == m_pAttributes )
		{
			if( Initialize(&m_TempAttributes) )
			{
				m_pAttributes = &m_TempAttributes;
			}
		}
		return m_pAttributes;
	}

private:
	SECURITY_ATTRIBUTES m_TempAttributes;
	PSECURITY_ATTRIBUTES m_pAttributes;
public:
	static BOOL Initialize(PSECURITY_ATTRIBUTES SecAttr)
	{
		/*初始化管道的属性*/
		LPCWSTR SSDLString = NULL;
		DWORD ErrorCode = 0;
		PSECURITY_DESCRIPTOR pSD = NULL;
		OSVERSIONINFOW VersionInfo = {0};
		VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
		if( !GetVersionExW( &VersionInfo ) )
		{
			return FALSE;
		}

		if( VersionInfo.dwMajorVersion >= 6 )
		{
			SSDLString = L"D:"
				L"(A;;GRGW;;;WD)"    /*Allow -> GENERIC_READ|GENERIC_WRITE -> Everyone*/
				L"(A;;GRGW;;;BA)"    /*Allow -> GENERIC_ALL -> Builtin (local ) administrators */
				L"S:"
				L"(ML;;NW;;;LW)"     /*Mandatory level -> WRITE_UP -> Low mandatory level */
				;
		}
		else
		{
			/*老系统*/
			SSDLString = L"D:"
				L"(A;;GRGW;;;WD)"    /*Allow -> GENERIC_READ|GENERIC_WRITE -> Everyone*/
				L"(A;;GRGW;;;BA)"    /*Allow -> GENERIC_ALL -> Builtin (local ) administrators */
				;
		}
		if ( !ConvertStringSecurityDescriptorToSecurityDescriptorW( SSDLString, SDDL_REVISION_1, &pSD, NULL ) )
		{
			DWORD ErrorCode;
			ErrorCode = GetLastError();
			return FALSE;
		}
		SecAttr->nLength = sizeof(SECURITY_ATTRIBUTES);
		SecAttr->bInheritHandle = FALSE;
		SecAttr->lpSecurityDescriptor = pSD;
		return TRUE;
	}
};


class pipe_acceptor
{
	typedef pipe_acceptor this_type;
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
		this_type::close();
	}

	void open(std::string const& pipeName)
	{
		++session_;
		pipe_name_ = "\\\\.\\Pipe\\" + pipeName;
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
		//do_async_accept<Handler>(stream,handler);

		get_io_service().post(
			boost::bind(&this_type::do_async_accept<Handler>,this,
			boost::ref(stream),boost::make_tuple(handler))
			);

	}

	void buffer_size(size_t newBufferSize)
	{
		buffer_size_ = (DWORD)newBufferSize;
	}

	size_t buffer_size(int newBufferSize)const
	{
		return buffer_size_;
	}

	boost::asio::io_service& get_io_service()
	{
		return service_;
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
			printf(" pipe server error: err=%d\n",kLastError);
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

		printf(" pipe server ok: err=%d\n",kLastError);
		stream.assign(pipe_handle_);
		pipe_handle_=INVALID_HANDLE_VALUE;
		overlapped.release();

		if (needCallback)
			boost::get<0>(handler)(boost::system::error_code());
	}
	void create_pipe( boost::system::error_code& ec )
	{
		CLowIntegritySecurityAttributes pipeSA;
		pipe_handle_ = ::CreateNamedPipeA(
			pipe_name_.c_str()
			, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED
			, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT
			, PIPE_UNLIMITED_INSTANCES
			, buffer_size_
			, buffer_size_
			, NMPWAIT_USE_DEFAULT_WAIT
			, &pipeSA
			);
		printf("create pipe\n");
		if(pipe_handle_ == INVALID_HANDLE_VALUE)
		{
			printf("create pipe error\n");
			ec=boost::system::error_code(::GetLastError(), boost::system::get_system_category());
		}
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

private:
	boost::asio::io_service&    service_;
	std::string                 pipe_name_;
	HANDLE                      pipe_handle_;
	DWORD                       buffer_size_;
	boost::atomic_int			session_;
};


class stream_pipe
	: public boost::asio::windows::stream_handle 
{
	typedef stream_pipe this_type;

public:
	explicit stream_pipe(boost::asio::io_service& service)
		: boost::asio::windows::stream_handle(service)
		, conn_timer_(service)
		, is_server_(true)
		, session_(0)
	{
	}
	virtual ~stream_pipe()
	{
		boost::system::error_code ec;
		this_type::close(ec);
	}
	void close()
	{
		boost::system::error_code ec;
		this_type::close(ec);
	}
	boost::system::error_code close(boost::system::error_code& ec)
	{
		++session_;
		conn_timer_.cancel(ec);
		if (is_server_&&native_handle()!=INVALID_HANDLE_VALUE)
			::DisconnectNamedPipe(native_handle());
		return boost::asio::windows::stream_handle::close(ec);
	}

	template<typename Handler>
	void async_connect(const std::string& pipeName, Handler handler)
	{
		++session_;
		get_io_service().post(
			boost::bind(&stream_pipe::async_connect<Handler>,pipeName,handler,_1,(int)session_)
			);
	}


	boost::system::error_code connect(const std::string& pipeName,int timeout_msec)
	{
		++session_;
		is_server_=false;
		std::string fullPipeName = "\\\\.\\Pipe\\" + pipeName;

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

protected:
	boost::system::error_code do_connect(const std::string& fullPipeName)
	{
		HANDLE pipe_handle = ::CreateFileA(
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
	template<typename Handler>
	void async_connect(const std::string& pipeName, Handler handler, 
		boost::system::error_code timer_error, int session)
	{
		if (session!=session_)
			return;

		if (timer_error) 
			return;

		boost::system::error_code ec=connect(pipeName);
		if (!ec)
		{
			conn_timer_.cancel(timer_error);
			handler(ec);
		}
		else
		{
			conn_timer_.expires_from_now(boost::chrono::seconds(100));
			conn_timer_.async_wait(
				boost::bind(&stream_pipe::async_connect<Handler>,pipeName,handler,_1,session)
				);
		}
	}
protected:
	bool is_server_;
	boost::asio::steady_timer conn_timer_;
	boost::atomic_int session_;
};

class message_pipe;
class shared_message
{
	friend class message_pipe;
	enum{kHEADER_LENGTH=4+4};//[message_length][message_id]
public:
	shared_message(int msgid,const void* _data, size_t _len)
		:data_(new char[_len+kHEADER_LENGTH])
	{
		memcpy((char*)data(),_data,_len);
		boost::int32_t* p=(boost::int32_t*)buffer();
		*p=(boost::int32_t)_len;
		++p;
		*p=msgid;
	}
	const char* data()const
	{
		if (!data_)
			return NULL;
		return data_.get()+kHEADER_LENGTH;
	}
	size_t size()const
	{
		return *(boost::int32_t*)(data_.get());
	}
	int id()const
	{
		return *(boost::int32_t*)(data_.get()+4);
	}
	operator bool () const 
	{
		return (bool)data_;
	}
private:
	shared_message()
		:data_(new char[kHEADER_LENGTH])
	{
		boost::int32_t* p=(boost::int32_t*)buffer();
		*p=0;
		++p;
		*p=0;
	}
	static size_t header_size()
	{
		return kHEADER_LENGTH;
	}
	size_t buffer_size()const
	{
		return size()+header_size();
	}
	void* buffer()
	{
		if (!data_) return NULL;
		return data_.get();
	}
	bool malloc_when_recvd_header()
	{
		int msgid=id();
	    int len=(int)size();
		if (len>0)
		{
			data_.reset(new char[kHEADER_LENGTH+len]);
			boost::int32_t* p=(boost::int32_t*)buffer();
			*p=len;
			++p;
			*p=msgid;
			return true;
		}
		else if(len==0)
		{
			return true;
		}
		return false;
	}
private:
	boost::shared_array<char> data_;
};

class message_pipe:public stream_pipe{
	typedef message_pipe this_type;
public:
	explicit message_pipe(boost::asio::io_service& service)
		:stream_pipe(service),sending_(false)
	{
	}
	virtual ~message_pipe()
	{
		this_type::close();
	}

	typedef boost::function<void(const shared_message,boost::system::error_code)> read_handler_type;
	void async_read(read_handler_type msg_handler)
	{
		read_header(msg_handler,(int)session_);
	}

	typedef boost::function<void(boost::system::error_code,size_t)> write_handler_type;
	void async_write(shared_message msg, write_handler_type h)
	{
		write(msg,h,(int)session_,false);
	}

	void close()
	{
		boost::system::error_code ec;
		this_type::close(ec);
	}
	boost::system::error_code close(boost::system::error_code& ec)
	{
		stream_pipe::close(ec);
		{
			while(send_que_.size())
				send_que_.pop_front();
			sending_=false;
		}
		return ec;
	}

private:
	void write(shared_message msg,write_handler_type handler,int session,bool dataFromQue)
	{
		if (session!=session_)
			return;

		if(!dataFromQue)
		{
			if (sending_)
			{
				send_que_.push_back(SendElem(msg,handler));
				return;
			}
			sending_=true;
		}
		boost::asio::async_write(*this,
			boost::asio::buffer(msg.buffer(),msg.buffer_size()),
			boost::asio::transfer_all(),
			boost::bind(&message_pipe::handle_write,this,_1,_2,msg,handler,session)
			);
	}

	void handle_write(boost::system::error_code ec, size_t len,const shared_message,
		write_handler_type handler,int session)
	{
		if (session!=session_)
			return;

		if (ec)
		{
			{
				while (!send_que_.empty())
					send_que_.pop_front();
				sending_=false;
				++session_;
			}
			if (handler)
				handler(ec,0);
			return;
		}
		else
		{
			{
				if(!send_que_.empty())
				{
					SendElem elm=send_que_.front();
					send_que_.pop_front();

					BOOST_ASSERT(sending_==true);
					write(elm.msg,elm.handler,session,true);
				}
				else
				{
					sending_=false;
				}
			}
			if (handler)
				handler(ec,len-shared_message::header_size());
		}
	}

	void read_header(read_handler_type handler,int session)
	{
		shared_message msg;
		boost::asio::async_read(*this,
			boost::asio::buffer(msg.buffer(),msg.header_size()),
			boost::asio::transfer_exactly(msg.header_size()),
			boost::bind(&message_pipe::handle_read_header,this,_1,_2,msg,handler,session)
			);
	}
	void handle_read_header(boost::system::error_code ec, size_t bytes_transfered,shared_message msg,
		read_handler_type handler,int session)
	{
		if (session!=session_)
			return;

		if (ec)
		{
			++session_;
			if (handler)
				handler(shared_message(),ec);
			return;
		}

		int dataSize=(int)msg.size();
		BOOST_ASSERT(bytes_transfered==shared_message::header_size());
		if (dataSize>0)
		{
			msg.malloc_when_recvd_header();
			boost::asio::async_read(*this,
				boost::asio::buffer((char*)msg.data(),dataSize),
				boost::asio::transfer_exactly(dataSize),
				boost::bind(&message_pipe::handle_read_body,this,_1,_2,msg,handler,session)
				);
		}
		else
		{
			handler(msg,ec);
		}
	}
	void handle_read_body(boost::system::error_code ec, size_t len,shared_message msg,
		read_handler_type handler,int session)
	{
		if (session!=session_)
			return;
		if (ec)
		{
			++session_;
			if (handler)
				handler(shared_message(),ec);
			return;
		}
		handler(msg,ec);
	}

private:
	struct SendElem
	{
		shared_message msg;
		write_handler_type handler;

		template<typename Handler>
		SendElem(shared_message _msg,Handler h)
			:msg(_msg),handler(h)
		{
		}
	};
	std::deque<SendElem> send_que_;
	boost::mutex send_que_mutex_;
	boost::atomic_bool sending_;
};

class io_service_thread{
public:
	virtual ~io_service_thread()
	{
		stop_thread();
	}
	boost::asio::io_service& get_io_service()
	{
		return io_service_;
	}
	void start_thread()
	{
		io_work_.reset(new boost::asio::io_service::work(io_service_));
		thread_=boost::thread(boost::bind(&io_service_thread::run,this));
	}
	void stop_thread()
	{
		io_work_.reset();
		io_service_.stop();
		thread_.join();
		io_service_.reset();
	}
private:
	void run()
	{
		boost::system::error_code ec;
		io_service_.run(ec);
		int x_for_debug=0;
	}

private:
	boost::asio::io_service io_service_;
	boost::scoped_ptr<boost::asio::io_service::work> io_work_;
	boost::thread thread_;
};

} // namespace pipe_ipc

