#pragma once
#include <atlbase.h>
#include <atlcom.h>
#include <boost/shared_ptr.hpp>
#include "pipe.h"
using namespace pipe_ipc;

typedef struct _IPCMsgBase {
	DWORD clientProcId;
	DWORD clientId;
	DWORD msgId;
} IPCAVEngMsgBase;

class CPipeIpcClient
{
	typedef CPipeIpcClient this_type;
public:
	CPipeIpcClient();
	~CPipeIpcClient();
	bool Init();
	void UnInit();
	int AsyncSendIpcMsg(DWORD dwMsgId, IPCAVEngMsgBase* pData, DWORD dwLen);
private:
	void HandleSendIpcMsg(this_type* self_holder,
			boost::shared_ptr<message_pipe> ipc,
			boost::system::error_code error,size_t dataLen
			);

	void HandleRecvdIpcMsg(this_type* self_holder,
			boost::shared_ptr<message_pipe> ipc,
			const shared_message data,
			boost::system::error_code error);

	void OnError(boost::shared_ptr<message_pipe> ipc,
			boost::system::error_code ec
			);

	static void HoldIpc(boost::shared_ptr<message_pipe> ipc){}

	int __OpenIpc();
	int __CloseIpc();
	boost::shared_ptr<io_service_thread> io_service_thread_;
	boost::shared_ptr<message_pipe> ipc_;
};