#pragma once

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

class io_service_thread
{
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