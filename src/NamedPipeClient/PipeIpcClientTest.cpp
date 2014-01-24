#include "PipeIpcClient.h"


int main(int argc, char* argv[])
{
	CPipeIpcClient client;
	client.Init();
	IPCAVEngMsgBase Param;
	while(1)
	{
		client.AsyncSendIpcMsg(111, &Param, sizeof(Param));
		Sleep(1000);
	}

	getchar();
	client.UnInit();
	return 0;
}

