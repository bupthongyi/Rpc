#include "ipc_message.h"


Message::Message(int routing_id, unsigned int type, PriorityValue priority)
	: Pickle(sizeof(Header))
{
		header()->routing = routing_id;
		header()->type = type;
		header()->flags = priority;
}

Message::Message(const char* data, int data_len) 
	: Pickle(data, data_len) 
{
}

Message::~Message()
{

}

Message::Message(const Message& other) : Pickle(other) 
{
}

Message& Message::operator=(const Message& other) 
{
	*static_cast<Pickle*>(this) = other;
	return *this;
}
