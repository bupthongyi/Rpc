#pragma once;


namespace zmq_rpc {

#ifndef int8_t
	typedef __int8 int8_t;
#endif
#ifndef int16_t
	typedef __int16 int16_t;
#endif
#ifndef int32_t
	typedef __int32 int32_t;
#endif
#ifndef int64_t
	typedef __int64 int64_t;
#endif
#ifndef uint8_t
	typedef unsigned __int8 uint8_t;
#endif
#ifndef uint16_t
	typedef unsigned __int16 uint16_t;
#endif
#ifndef uint32_t
	typedef unsigned __int32 uint32_t;
#endif
#ifndef uint64_t
	typedef unsigned __int64 uint64_t;
#endif

static inline char* string_as_array(std::string* str) 
{
	//return str->empty() ? NULL : &*str->begin();
	return str->empty() ? NULL : &((*str)[0]);
}

}