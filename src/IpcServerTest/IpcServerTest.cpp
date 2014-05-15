#include <windows.h>
#include <tchar.h>
#include "ipc.h"

int _tmain(int argc, _TCHAR* argv[])
{
	boost::shared_ptr<IpcService> service(new IpcService(L"lihongyi01"));
	service->Start();

	system("pause");

	service->Stop();
	return 0;
}

