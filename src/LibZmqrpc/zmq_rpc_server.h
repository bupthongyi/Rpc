#pragma once

#include "zmq_rpc.h"

namespace zmq_rpc {

class RPCServer
{
public:
	RPCServer();
	~RPCServer();
	bool RegisterProtocol(const char* method, Service* service);
	bool StartServer(int port, std::string host, int call_back_threads = 4);
	bool StartServer(int call_back_threads = 4);
	void StopServer();
	bool StartService();
	bool running() const {return running_;}
	const std::string& server_ip() const {return server_ip_;}
	int server_port() {return server_port_;}
private:
	zmq_rpc::Connection *conn_;
	HANDLE working_thread_;
	bool running_;
	bool stop_;
	int server_port_;
	std::string server_ip_;
	int callback_threads_;
};

}