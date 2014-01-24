#include "serializer.h"
#include "google/protobuf/io/coded_stream.h"
namespace zmq_rpc {

// ------------------------------------------------------------
// wrapper protobuf serialize and deserialize method.
// !!!only works with memory buffer.
template <class T>
bool SerializeToWriteBuffer(const T* obj, WriteableByteArray* bytes) {
	if(!bytes) {
		return false;
	}
	size_t size = obj->ByteSize();
	if(!serialize(size, bytes)) {
		return false;
	}
	Byte* serial_buf = bytes->allocate(size);
	if(!serial_buf) {
		return false;
	}
	if(!obj->SerializeToArray(serial_buf, size)) {
		return false;
	}
	return true;
}

template <class T>
bool DeserizlizeFromReadArray(T* obj, ReadableByteArray* bytes) {
	if(!bytes) {
		return false;
	}
	size_t size;
	if(!deserialize(&size, bytes)) {
		return false;
	}
	const Byte* data = bytes->remain(NULL);
	google::protobuf::io::CodedInputStream input(data, size);
	if(!obj->ParseFromCodedStream(&input)) {
		return false;
	}
	bytes->read(NULL, size);
	return true;
}

} // namespace proto_serializer