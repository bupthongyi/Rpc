#pragma once;
#include "proto_rpc.h"
#include "zmq_rpc_server.h"
#include "proto/Message.pb.h"

extern const char* kOK;
extern const char* kFail;


extern const char* kA2BRpcCallMethod;
typedef zmq_rpc::RPCService<TestRequestAB, TestResponseAB> A2BRpcCallProtocol;
#define A_B_RPC_CALL handleRequest

extern const char* kA2CRpcCallMethod;
typedef zmq_rpc::RPCService<TestRequestAC, TestResponseAC> A2CRpcCallProtocol;
#define A_C_RPC_CALL handleRequest

extern const char* kB2ARpcCallMethod;
typedef zmq_rpc::RPCService<TestRequestBA, TestResponseBA> B2ARpcCallProtocol;
#define B_A_RPC_CALL handleRequest

extern const char* kB2CRpcCallMethod;
typedef zmq_rpc::RPCService<TestRequestBC, TestResponseBC> B2CRpcCallProtocol;
#define B_C_RPC_CALL handleRequest

extern const char* kC2ARpcCallMethod;
typedef zmq_rpc::RPCService<TestRequestCA, TestResponseCA> C2ARpcCallProtocol;
#define C_A_RPC_CALL handleRequest

extern const char* kC2BRpcCallMethod;
typedef zmq_rpc::RPCService<TestRequestCB, TestResponseCB> C2BRpcCallProtocol;
#define C_B_RPC_CALL handleRequest