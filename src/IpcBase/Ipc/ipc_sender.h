#pragma once

class Message;
class Sender {
public:
	// Sends the given IPC message.  The implementor takes ownership of the
	// given Message regardless of whether or not this method succeeds.  This
	// is done to make this method easier to use.  Returns true on success and
	// false otherwise.
	virtual bool Send(boost::shared_ptr<Message> msg) = 0;

protected:
	virtual ~Sender() {}
};