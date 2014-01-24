#include "RPCServerB.h"

std::string RPCServerB::A_B_RPC_CALL(const TestRequestAB* req, TestResponseAB* ack) 
{
	static int i = 0;

	ack->set_id(req->id());
	ack->set_debug_info(req->debug_info());

	i++;
	if(i%1000 == 0)
	{
		printf("server A_B_RPC_CALL %d\n", i);
	}
	return kOK;
}


std::string RPCServerB::C_B_RPC_CALL(const TestRequestCB* req, TestResponseCB* ack) 
{
	static int i = 0;

	ack->set_id(req->id());
	ack->set_debug_info(req->debug_info());

	i++;
	if(i%1000 == 0)
	{
		printf("server C_B_RPC_CALL %d\n", i);
	}
	return kOK;
}
