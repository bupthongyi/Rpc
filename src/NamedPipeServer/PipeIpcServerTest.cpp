#include "PipeIpcServer.h"

int main(int argc, char* argv[])
{
	CPipeIpcServer server;
	server.Init();
	getchar();
	server.UnInit();
	return 0;
}

