#pragma once

#include "pickle.h"

class Message : public Pickle 
{
public:
	enum PriorityValue {
		PRIORITY_LOW = 1,
		PRIORITY_NORMAL,
		PRIORITY_HIGH
	};

	// Bit values used in the flags field.
	// Upper 24 bits of flags store a reference number, so this enum is limited to
	// 8 bits.
	enum {
		PRIORITY_MASK     = 0x03,  // Low 2 bits of store the priority value.
		SYNC_BIT          = 0x04,
		REPLY_BIT         = 0x08,
		REPLY_ERROR_BIT   = 0x10,
		UNBLOCK_BIT       = 0x20,
		PUMPING_MSGS_BIT  = 0x40,
		HAS_SENT_TIME_BIT = 0x80,
	};

	Message(int routing_id, unsigned int type, PriorityValue priority);
	Message(const char* data, int data_len);
	virtual ~Message();

	Message(const Message& other);
	Message& operator=(const Message& other);

	unsigned int type() const 
	{
		return header()->type;
	}

	int routing_id() const 
	{
		return header()->routing;
	}

	static const char* FindNext(const char* range_start, const char* range_end) 
	{
		return Pickle::FindNext(sizeof(Header), range_start, range_end);
	}

protected:
#pragma pack(push, 4)
	struct Header : Pickle::Header {
		int routing;  // ID of the view that this message is destined for
		unsigned int type;    // specifies the user-defined message type
		unsigned int flags;   // specifies control flags for the message
	};
#pragma pack(pop)

	Header* header() 
	{
		return headerT<Header>();
	}
	const Header* header() const 
	{
		return headerT<Header>();
	}
};