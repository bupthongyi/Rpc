#pragma once;
#include <string>
#include "RPCConst.h"

class RPCServerA : public zmq_rpc::RPCServer,
	public B2ARpcCallProtocol
{
public:
	RPCServerA()
	{
		RegisterProtocol(kB2ARpcCallMethod, (B2ARpcCallProtocol*)this);
		RegisterProtocol(kC2ARpcCallMethod, (C2ARpcCallProtocol*)this);
	}

	std::string B_A_RPC_CALL(const TestRequestBA* req, TestResponseBA* ack);
	std::string C_A_RPC_CALL(const TestRequestCA* req, TestResponseCA* ack);
private:
};