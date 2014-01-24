#include "serializer.h"
#include "utils.h"

namespace zmq_rpc {

bool serialize(const uint64_t u, WriteableByteArray* bytes) {
	return bytes->write(reinterpret_cast<const Byte*>(&u), sizeof(u));
}

bool deserialize(uint64_t* u, ReadableByteArray* bytes) {
	return bytes->read(reinterpret_cast<Byte*>(u), sizeof(*u));
}

bool serialize(const int64_t u, WriteableByteArray* bytes) {
	return bytes->write(reinterpret_cast<const Byte*>(&u), sizeof(u));
}

bool deserialize(int64_t* u, ReadableByteArray* bytes) {
	return bytes->read(reinterpret_cast<Byte*>(u), sizeof(*u));
}

bool serialize(const uint32_t u, WriteableByteArray* bytes) {
	return bytes->write(reinterpret_cast<const Byte*>(&u), sizeof(u));
}

bool deserialize(uint32_t* u, ReadableByteArray* bytes) {
	return bytes->read(reinterpret_cast<Byte*>(u), sizeof(*u));
}

bool serialize(const int32_t u, WriteableByteArray* bytes) {
	return bytes->write(reinterpret_cast<const Byte*>(&u), sizeof(u));
}

bool deserialize(int32_t* u, ReadableByteArray* bytes) {
	return bytes->read(reinterpret_cast<Byte*>(u), sizeof(*u));
}

bool serialize(const char u, WriteableByteArray* bytes) {
	return bytes->write(reinterpret_cast<const Byte*>(&u), sizeof(u));
}

bool deserialize(char* u, ReadableByteArray* bytes) {
	return bytes->read(reinterpret_cast<Byte*>(u), sizeof(*u));
}

bool serialize(const std::string& u, WriteableByteArray* bytes) {
	if(!serialize(u.size(), bytes)) {
		return false;
	}
	return bytes->write(reinterpret_cast<const Byte*>(u.data()), u.size());
}

bool deserialize(std::string* u, ReadableByteArray* bytes) {
	size_t size;
	if(!deserialize(&size, bytes)) {
		return false;
	}
	//DEBUG("std::string size:%zu", size);
	u->resize(size);
	return bytes->read(reinterpret_cast<Byte*>(string_as_array(u)), size);
}

} // namespace serializer