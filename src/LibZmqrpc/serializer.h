#pragma once
#include <string>
#include "utils.h"
#include "byte_array.h"


namespace zmq_rpc {

// ------------------------------------------------------------
class Serializable {
public:
	virtual bool serialize(WriteableByteArray* bytes) const = 0;
	virtual bool deserialize(ReadableByteArray* bytes) = 0;
	virtual ~Serializable() {}
}; // class Serializable

// ------------------------------------------------------------
template<typename T>
class Stream {
public:
	bool out(const T& x, WriteableByteArray* bytes) const {
		return x.serialize(bytes);
	}
	bool in(T* x, ReadableByteArray* bytes) {
		return x->deserialize(bytes);
	}
}; // class Stream

// ------------------------------------------------------------
#define DSTREAM_CC_COMMON_SERIALIZER_ENUM_TYPE(type)                    \
template<>                                                            \
class Stream<type> {                                                  \
public:                                                              \
bool out(const type& x,WriteableByteArray* bytes) const{             \
return serialize(x,bytes);                                         \
}                                                                    \
bool in(type* x,ReadableByteArray* bytes) const{                     \
return deserialize(x,bytes);                                       \
}                                                                    \
}
bool serialize(const uint64_t u, WriteableByteArray* bytes);
bool deserialize(uint64_t* u, ReadableByteArray* bytes);
DSTREAM_CC_COMMON_SERIALIZER_ENUM_TYPE(uint64_t);

bool serialize(const int64_t u, WriteableByteArray* bytes);
bool deserialize(int64_t* u, ReadableByteArray* bytes);
DSTREAM_CC_COMMON_SERIALIZER_ENUM_TYPE(int64_t);

bool serialize(const uint32_t u, WriteableByteArray* bytes);
bool deserialize(uint32_t* u, ReadableByteArray* bytes);
DSTREAM_CC_COMMON_SERIALIZER_ENUM_TYPE(uint32_t);

bool serialize(const int32_t u, WriteableByteArray* bytes);
bool deserialize(int32_t* u, ReadableByteArray* bytes);
DSTREAM_CC_COMMON_SERIALIZER_ENUM_TYPE(int32_t);

bool serialize(const char u, WriteableByteArray* bytes);
bool deserialize(char* u, ReadableByteArray* bytes);
DSTREAM_CC_COMMON_SERIALIZER_ENUM_TYPE(char);

bool serialize(const std::string& u, WriteableByteArray* bytes);
bool deserialize(std::string* u, ReadableByteArray* bytes);
DSTREAM_CC_COMMON_SERIALIZER_ENUM_TYPE(std::string);

#undef DSTREAM_CC_COMMON_SERIALIZER_ENUM_TYPE

} // namespace serializer