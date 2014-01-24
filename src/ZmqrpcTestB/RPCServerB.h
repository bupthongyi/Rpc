#pragma once;
#include <string>
#include "RPCConst.h"

class RPCServerB : public zmq_rpc::RPCServer,
	public A2BRpcCallProtocol,
	public C2BRpcCallProtocol
{
public:
	RPCServerB()
	{
		RegisterProtocol(kA2BRpcCallMethod, (A2BRpcCallProtocol*)this);
		RegisterProtocol(kC2BRpcCallMethod, (C2BRpcCallProtocol*)this);
	}

	std::string A_B_RPC_CALL(const TestRequestAB* req, TestResponseAB* ack);
	std::string C_B_RPC_CALL(const TestRequestCB* req, TestResponseCB* ack);
private:
};