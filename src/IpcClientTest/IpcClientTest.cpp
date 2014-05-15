#include <windows.h>
#include <tchar.h>

#include "test_listener.h"

int _tmain(int argc, _TCHAR* argv[])
{
	//Message* msg = new Message(0, 2, Message::PRIORITY_NORMAL);
	//msg->WriteInt64(time(NULL));
	//msg->WriteInt(999);
	//msg->WriteString("lihongyitest");

	//PickleIterator iter(*msg);
	//long long time_internal;
	//bool result = iter.ReadInt64(&time_internal);
	//int msgid;
	//result = iter.ReadInt(&msgid);
	//std::string reflected_payload;
	//result = iter.ReadString(&reflected_payload);

	//if(argc != 3)
	//{
	//	return 0;
	//}
	//int myself_id = _wtoi(argv[1]);
	//int send_to_id = _wtoi(argv[2]);

	int myself_id = GetCurrentProcessId();
	int send_to_id = GetCurrentProcessId();

	TestListener listener;
	Channel channel(L"lihongyi01", 10000);
	channel.RegisterListener(&listener, myself_id);
	channel.Connect();

	while(true)
	{
		boost::shared_ptr<Message> hello1(new Message(send_to_id, NOMAL_MESSAGE_TYPE, Message::PRIORITY_NORMAL));
		hello1->WriteInt64(time(NULL));
		hello1->WriteInt(999);
		hello1->WriteString("lihongyitest1111111111111111111111111111111111111111111111111111111111111111111111111111111");
		channel.Send(hello1);
		Sleep(20);
	}

	//system("pause");
	channel.Close();

	return 0;
}

