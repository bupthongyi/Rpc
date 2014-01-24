#pragma once;
#include <string>
#include "RPCConst.h"

class RPCServerC : public zmq_rpc::RPCServer,
	public A2CRpcCallProtocol,
	public B2CRpcCallProtocol
{
public:
	RPCServerC()
	{
		RegisterProtocol(kA2CRpcCallMethod, (A2CRpcCallProtocol*)this);
		RegisterProtocol(kB2CRpcCallMethod, (B2CRpcCallProtocol*)this);
	}

	std::string A_C_RPC_CALL(const TestRequestAC* req, TestResponseAC* ack);
	std::string B_C_RPC_CALL(const TestRequestBC* req, TestResponseBC* ack);
private:
};