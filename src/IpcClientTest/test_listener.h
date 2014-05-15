#pragma once

#include "ipc.h"

class TestListener : public Listener 
{
public:
	TestListener()
	{
	}

	virtual ~TestListener() 
	{
	}

	virtual bool OnMessageReceived(const Message& message)
	{
		/*printf("routing_id == %d\ttype = %u\t", message.routing_id(), message.type());
		PickleIterator iter(message);
		long long time_internal;
		bool result = iter.ReadInt64(&time_internal);
		if(result)
		{
			printf("time = %lld\t",time_internal);
		}
		int msgid;
		result = iter.ReadInt(&msgid);
		if(result)
		{
			printf("int = %d\t",msgid);
		}
		std::string reflected_payload;
		result = iter.ReadString(&reflected_payload);
		if(result)
		{
			printf("reflected_payload = %s",reflected_payload.c_str());
		}
		printf("\n");*/
		return true;
	}
};
