#include "RPCServerC.h"

std::string RPCServerC::A_C_RPC_CALL(const TestRequestAC* req, TestResponseAC* ack) 
{
	static int i = 0;

	ack->set_id(req->id());
	ack->set_debug_info(req->debug_info());

	i++;
	if(i%1000 == 0)
	{
		printf("server A_C_RPC_CALL %d\n", i);
	}
	return kOK;
}


std::string RPCServerC::B_C_RPC_CALL(const TestRequestBC* req, TestResponseBC* ack) 
{
	static int i = 0;

	ack->set_id(req->id());
	ack->set_debug_info(req->debug_info());

	i++;
	if(i%1000 == 0)
	{
		printf("server B_C_RPC_CALL %d\n", i);
	}
	return kOK;
}