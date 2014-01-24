#pragma once
#include <boost/shared_ptr.hpp>
#include "pipe.h"
using namespace pipe_ipc;

class CServer
{
typedef CServer this_type;
public:
	void SetIpc(boost::shared_ptr<message_pipe>  pipe);
	void HandleIpcMsg(this_type* holder,
		boost::shared_ptr<message_pipe> ipc,
		const shared_message data,
		boost::system::error_code error);

	boost::shared_ptr<message_pipe> pipe_;
};

class CPipeIpcServer
{
typedef CPipeIpcServer this_type;
public:
	CPipeIpcServer();
	~CPipeIpcServer();
	bool Init();
	void UnInit();

private:
	void HandleAccept(boost::shared_ptr<message_pipe> pipe,boost::system::error_code ec);
	//void HandleIpcMsg(this_type* holder,
	//	boost::shared_ptr<message_pipe> ipc,
	//	const shared_message data,
	//	boost::system::error_code error);

	io_service_thread io_service_thread_;
	boost::shared_ptr<pipe_acceptor> pipe_acceptor_;
	//boost::shared_ptr<message_pipe> pipe_;
};