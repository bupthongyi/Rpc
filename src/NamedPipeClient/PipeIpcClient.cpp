#include "PipeIpcClient.h"

CPipeIpcClient::CPipeIpcClient()
{
}

CPipeIpcClient::~CPipeIpcClient()
{
}

bool CPipeIpcClient::Init()
{
	__OpenIpc();
	return 0;
}

void CPipeIpcClient::UnInit()
{
	__CloseIpc();
}

int CPipeIpcClient::AsyncSendIpcMsg(DWORD dwMsgId, IPCAVEngMsgBase* pData, DWORD dwLen )
{
	if (NULL == pData)
		return S_FALSE;

	pData->clientProcId = ::GetCurrentProcessId();
	pData->clientId = 200;
	pData->msgId = dwMsgId;

	shared_message buf(dwMsgId,pData,dwLen);
	{
		if (!ipc_)
		{
			return -1;
		}
		ipc_->async_write(buf,
			boost::bind(&this_type::HandleSendIpcMsg,this,this,ipc_,_1,_2)
			);
	}

	return 0;
}

int CPipeIpcClient::__OpenIpc()
{
	printf("open ipc\n");
	if (!io_service_thread_)
	{
		io_service_thread_.reset(new io_service_thread);
		io_service_thread_->start_thread();
	}
	if (!ipc_)
		ipc_.reset(new message_pipe(io_service_thread_->get_io_service()));
	if (ipc_->is_open())
	{
		printf("ipc_ is_open\n");
		return S_OK;
	}
	boost::system::error_code ec = ipc_->connect("lihongyiTest1",1000);
	if (ec)
	{
		printf("ipc_ connect error\n");
		boost::system::error_code error;
		ipc_->close(error);

		io_service_thread_->get_io_service().post(
			boost::bind(&this_type::OnError,this,ipc_,ec)
			);

		return -1;
	}
	else
	{
		printf("ipc_ connect ok\n");
		ipc_->async_read(boost::bind(&this_type::HandleRecvdIpcMsg,this,
			this,ipc_,_1,_2));
		return 0;
	}
}


int CPipeIpcClient::__CloseIpc()
{
	if (ipc_)
	{
		boost::system::error_code ec;
		ipc_->close(ec);
		ipc_->get_io_service().post(boost::bind(&CPipeIpcClient::HoldIpc,ipc_));
		ipc_.reset();
	}

	boost::shared_ptr<io_service_thread> serviceThread=io_service_thread_;
	io_service_thread_.reset();
	if (serviceThread)
	{
		serviceThread->stop_thread();
	}
	return 0;
}


void CPipeIpcClient::OnError(boost::shared_ptr<message_pipe> ipc,
		boost::system::error_code ec)
{
}


void CPipeIpcClient::HandleSendIpcMsg(this_type* self_holder,
		boost::shared_ptr<message_pipe> ipc,
		boost::system::error_code error,
		size_t dataLen)
{
	if (ipc!=ipc_)
		return;

	if (error)
		OnError(ipc,error);
}

void CPipeIpcClient::HandleRecvdIpcMsg(this_type* self_holder,
		boost::shared_ptr<message_pipe> ipc,
		const shared_message data,  
		boost::system::error_code error)
{
	if (ipc!=ipc_)
		return;

	if (error)
	{
		OnError(ipc,error);
		return;
	}

	ipc->async_read(
		boost::bind(&this_type::HandleRecvdIpcMsg,this,self_holder,ipc,_1,_2)
		);

	const char* pData=data?data.data():NULL;
	if (!pData)
		return;

	int msgID=data.id();
}