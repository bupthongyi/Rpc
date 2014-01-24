#include <process.h>
#include "zmq_rpc_server.h"

namespace zmq_rpc {
#define TRY_START_PORT 5000
#define TRY_END_PORT 60000

const char* kDefaultProtocol = "tcp";

std::string GetUri(const std::string& protocol, const std::string& host, int port) {
	char  uri[400];
	sprintf(uri, "%s://%s:%d", protocol.c_str(), host.c_str(), port);
	return uri;
}

std::string GetUri(const std::string& host, int port) {
	return GetUri(kDefaultProtocol, host, port);
}

static unsigned int __stdcall RPCServerWorkThread(void* ptr)
{
	RPCServer* server = (RPCServer*)(ptr);
	if (!server->StartService()) 
	{
		return 0;
	}
	return 0;
}

RPCServer::RPCServer()
{
	running_ = false;
	stop_ = false;
	conn_ = NULL;
	server_port_ = -1;
	working_thread_ = NULL;
}

RPCServer::~RPCServer() 
{
	 StopServer();
}

bool RPCServer::RegisterProtocol(const char* method, Service* service)
{
	if (NULL == conn_) 
	{
		Context::init();
		if(NULL == (conn_ = new Connection)) 
		{
			return false;
		}
	}
	conn_->registerService(method, service);
	return true;
}

bool RPCServer::StartServer(int port, std::string host, int call_back_threads)
{
	if (running_) 
	{
		return true;
	}
	if (stop_) 
	{
		stop_ = false;
	}
	if (NULL == conn_) 
	{
		Context::init();
		if(NULL == (conn_ = new Connection)) 
		{
			return false;
		}
	}
	if (host == "*") 
	{
		//if (!get_localhost_ip(&server_ip_))
		//{
		//	return false;
		//}
		server_ip_ = "127.0.0.1";
	} 
	else 
	{
		server_ip_ = host;
	}
	server_port_ = port;
	callback_threads_ = call_back_threads;
	running_ = true;
	working_thread_ = (HANDLE)_beginthreadex(
												NULL, 
												0, 
												RPCServerWorkThread, 
												this, 
												0, 
												NULL);
	if(working_thread_ == NULL) 
	{
		return false;
	}
	while(running_ && !conn_->working()) 
	{
		// wait time for server start
		//Sleep(3000);
	}
	return running_;
}

bool RPCServer::StartServer(int call_back_threads)
{
	return StartServer(0, "*", call_back_threads);
}

bool RPCServer::StartService()
{
	if (server_port_ == 0) 
	{
		for (server_port_ = TRY_START_PORT; server_port_ <= TRY_END_PORT; ++server_port_) 
		{
			conn_->serve(GetUri(server_ip_, server_port_).c_str(), callback_threads_);
			if (stop_) 
			{
				return true;
			}
		}
	} 
	else 
	{
		conn_->serve(GetUri(server_ip_, server_port_).c_str(), callback_threads_);
		if (stop_) 
		{
			return true;
		}
	}
	return (running_ = false);
}

void RPCServer::StopServer()
{
	stop_	= true;
	if (working_thread_ != 0 && NULL != conn_) 
	{
		HANDLE working_thread_tmp = working_thread_;
		Connection* conn_tmp = conn_;
		working_thread_	= 0;
		conn_->stop();
		conn_ = NULL;
		running_ = false;
		DWORD dwExitCode = ::WaitForSingleObject(working_thread_tmp, INFINITE);
		if ( WAIT_OBJECT_0 == dwExitCode )
		{
			CloseHandle(working_thread_tmp);
		}
		delete conn_tmp;
		return;
	}
	if (NULL != conn_) 
	{
		conn_->stop();
		delete conn_;
		conn_ = NULL;
	}
}

}