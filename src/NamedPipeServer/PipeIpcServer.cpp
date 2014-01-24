#include "PipeIpcServer.h"

void CServer::SetIpc(boost::shared_ptr<message_pipe>  pipe)
{

	pipe_=pipe;
	pipe->async_read(
		boost::bind(&this_type::HandleIpcMsg,this,this,pipe,_1,_2)
		);
}

void CServer::HandleIpcMsg(this_type* holder,
			boost::shared_ptr<message_pipe> pipe,
			const shared_message data,
			boost::system::error_code error)
{
	if (!pipe)
		return;

	if (pipe!=pipe_)
	{
		return;
	}

	if (error)
	{
		return;
	}

	int msgID=data.id();
	printf("receiverd  msgID = %d\n", msgID);

	pipe_->async_read(boost::bind(&this_type::HandleIpcMsg,this,holder,pipe_,_1,_2));
}

CPipeIpcServer::CPipeIpcServer()
{
	pipe_acceptor_.reset(new pipe_acceptor(io_service_thread_.get_io_service()));
}

CPipeIpcServer::~CPipeIpcServer()
{
}

bool CPipeIpcServer::Init()
{
	Sleep(40);
	io_service_thread_.start_thread();
	pipe_acceptor_->open("lihongyiTest1");

	boost::shared_ptr<message_pipe> ipc(new message_pipe(io_service_thread_.get_io_service()));
	pipe_acceptor_->async_accept(*ipc,boost::bind(&this_type::HandleAccept,this,ipc,_1));
	return true;
}

void CPipeIpcServer::UnInit()
{
	pipe_acceptor_->close();
	io_service_thread_.stop_thread();
}

void CPipeIpcServer::HandleAccept(boost::shared_ptr<message_pipe> pipe,boost::system::error_code ec)
{
	printf(" CPipeIpcServer::HandleAccept \n");
	boost::shared_ptr<message_pipe> ipc(new message_pipe(io_service_thread_.get_io_service()));
	if (ec) 
	{
		Sleep(1);//wait for a moment
		pipe_acceptor_->async_accept(*ipc,boost::bind(&this_type::HandleAccept,this,ipc,_1));
	}
	else
	{
		pipe_acceptor_->async_accept(*ipc,boost::bind(&this_type::HandleAccept,this,ipc,_1));
		CServer *pServer =  new CServer;
		if (pServer)
		{
			pServer->SetIpc(pipe);
		}
	}
}