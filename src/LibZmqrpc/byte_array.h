#pragma once

namespace zmq_rpc {

typedef unsigned char Byte;
typedef size_t ByteSize;

// ------------------------------------------------------------
class ReadableByteArray {
public:
	ReadableByteArray(const Byte* bytes, ByteSize size);
	virtual ~ReadableByteArray() {}
	// abstract interface.
	virtual bool read(Byte* bytes, ByteSize size);
	// interface only works with internal implementation is memory buffer.
	virtual const Byte* data(ByteSize* size) const;
	virtual const Byte* remain(ByteSize* size) const;
private:
	const Byte* bytes_;
	ByteSize size_;
	ByteSize cur_;
}; // class ReadableByteArray

// ------------------------------------------------------------
class WriteableByteArray {
public:
	// if 'user_free'==true, user has to free 'data' manually
	// in some situation, it's much efficient.
	// and to now, this feature can be applied to 'rpc'.
	WriteableByteArray(bool user_free = false);
	virtual ~WriteableByteArray();
	// abstract interface.
	virtual bool write(const Byte* bytes, ByteSize size);
	virtual const Byte* data(ByteSize* size) const;
	virtual Byte* allocate(ByteSize size);
private:
	bool user_free_;
	Byte* bytes_;
	ByteSize size_;
	ByteSize cur_;
	static const int kSmallBufferSize = 128;
	Byte small_[kSmallBufferSize];
}; // class WriteableByteArray

}