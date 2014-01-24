#include "RPCServerA.h"


std::string RPCServerA::B_A_RPC_CALL(const TestRequestBA* req, TestResponseBA* ack) 
{
	static int i = 0;

	ack->set_id(req->id());
	ack->set_debug_info(req->debug_info());

	i++;
	if(i%1000 == 0)
	{
		printf("server B_A_RPC_CALL %d\n", i);
	}
	return kOK;
}

std::string RPCServerA::C_A_RPC_CALL(const TestRequestCA* req, TestResponseCA* ack) 
{
	static int i = 0;

	ack->set_id(req->id());
	ack->set_debug_info(req->debug_info());

	i++;
	if(i%1000 == 0)
	{
		printf("server C_A_RPC_CALL %d\n", i);
	}
	return kOK;
}

