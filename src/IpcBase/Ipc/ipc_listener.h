#pragma once

class Message;
class Listener 
{
public:
	virtual bool OnMessageReceived(const Message& message) = 0;

protected:
	virtual ~Listener() {}
};