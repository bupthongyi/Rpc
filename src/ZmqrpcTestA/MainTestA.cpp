#include <process.h>
#include <string>
#include "RPCServerA.h"

unsigned int __stdcall WorkThread(void *arg)
{
	zmq_rpc::Connection conn;
	if(conn.connect("tcp://127.0.0.1:5002") < 0) 
	{
		return 1;
	}

	zmq_rpc::RPCRequest< TestRequestAC > req;
	zmq_rpc::RPCResponse< TestResponseAC> res;
	req.set_method(kA2CRpcCallMethod);
	req.set_id(1);
	char debug_info_string[256] = {0};
	sprintf(debug_info_string, "%s,","send form Test A");
	req.set_debug_info(debug_info_string);

	std::string resvalue;
	int i = 0;
	while(1)
	{
		int rpc_call_ret = conn.call(&req, &res, 1000);
		if(rpc_call_ret == -1)
		{
			printf("error\n");
			conn.reconnect();
		}

		i++;
		if(i%1000 == 0)
		{
			printf("send %d\n",i);
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	RPCServerA server;
	server.StartServer(5000, "127.0.0.1", 4);

	HANDLE ThreadHandle = (HANDLE)_beginthreadex(
		NULL, 
		0, 
		WorkThread, 
		NULL, 
		0, 
		NULL);

	::WaitForSingleObject(ThreadHandle, INFINITE);
	CloseHandle(ThreadHandle);
	return 0;
}

