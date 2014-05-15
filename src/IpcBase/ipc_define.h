#pragma once

enum SpecialRoutingIDs 
{
	// indicates that we don't have a routing ID yet.
	MSG_ROUTING_NONE = -2,

	MSG_ROUTING_TEST1 = 1001,
	MSG_ROUTING_TEST2 = 1002,
	MSG_ROUTING_TEST3 = 1003,

	// indicates a general message not sent to a particular tab.
	MSG_ROUTING_CONTROL = 0x7FFFFFFF,
};

enum 
{
	NOMAL_MESSAGE_TYPE = 0,
	REGISTER_MESSAGE_TYPE = 0xFFFFFFFF-1,
	HELLO_MESSAGE_TYPE = 0xFFFFFFFF,
};