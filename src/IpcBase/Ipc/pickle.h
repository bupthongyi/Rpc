#pragma once
#include <windows.h>
#include <string>

#include <boost/cstdint.hpp>

typedef signed char			int8;
typedef unsigned char		uint8;
typedef short				int16;
typedef unsigned short		uint16;
typedef __int32				int32;
typedef unsigned int		uint32;
typedef long long			int64;
typedef unsigned long long	uint64;

class Pickle;

class PickleIterator 
{
public:
	PickleIterator() : read_ptr_(NULL), read_end_ptr_(NULL) {}
	explicit PickleIterator(const Pickle& pickle);

	bool ReadBool(bool* result);
	bool ReadInt(int* result);
	bool ReadLong(long* result);
	bool ReadUInt16(uint16* result);
	bool ReadUInt32(uint32* result);
	bool ReadInt64(int64* result);
	bool ReadUInt64(uint64* result);
	bool ReadFloat(float* result);
	bool ReadString(std::string* result);
	bool ReadWString(std::wstring* result);
	bool ReadData(const char** data, int* length);
	bool ReadBytes(const char** data, int length);

private:
	static size_t AlignInt(size_t i, int alignment) 
	{
		return i + (alignment - (i % alignment)) % alignment;
	}

	template <typename Type>
	inline bool ReadBuiltinType(Type* result);

	template<typename Type>
	inline const char* GetReadPointerAndAdvance();

	const char* GetReadPointerAndAdvance(int num_bytes);

	inline const char* GetReadPointerAndAdvance(int num_elements,size_t size_element);


	const char* read_ptr_;
	const char* read_end_ptr_;
};

class Pickle 
{
public:
	explicit Pickle(int header_size);
	Pickle(const char* data, int data_len);
	virtual ~Pickle();

	Pickle(const Pickle& other);
	Pickle& operator=(const Pickle& other);

	// Returns the size of the Pickle's data.
	size_t size() const { return header_size_ + header_->payload_size; }

	// Returns the data for this Pickle.
	const void* data() const { return header_; }

	bool WriteBool(bool value) {
		return WriteInt(value ? 1 : 0);
	}
	bool WriteInt(int value) {
		return WritePOD(value);
	}
	bool WriteLong(long value) {
		return WritePOD(value);
	}
	bool WriteUInt16(uint16 value) {
		return WritePOD(value);
	}
	bool WriteUInt32(uint32 value) {
		return WritePOD(value);
	}
	bool WriteInt64(int64 value) {
		return WritePOD(value);
	}
	bool WriteUInt64(uint64 value) {
		return WritePOD(value);
	}
	bool WriteFloat(float value) {
		return WritePOD(value);
	}

	bool WriteString(const std::string& value);
	bool WriteWString(const std::wstring& value);
	bool WriteData(const char* data, int length);
	bool WriteBytes(const void* data, int length);

	struct Header 
	{
		unsigned int payload_size;  // Specifies the size of the payload.
	};

	template <class T>
	T* headerT() 
	{
		return static_cast<T*>(header_);
	}
	template <class T>
	const T* headerT() const 
	{
		return static_cast<const T*>(header_);
	}

	size_t payload_size() const { return header_->payload_size; }

	const char* payload() const 
	{
		return reinterpret_cast<const char*>(header_) + header_size_;
	}

	const char* end_of_payload() const 
	{
		// This object may be invalid.
		return header_ ? payload() + payload_size() : NULL;
	}

	static const char* FindNext(size_t header_size,
								const char* range_start,
								const char* range_end);

private:
	char* mutable_payload() 
	{
		return reinterpret_cast<char*>(header_) + header_size_;
	}

	size_t capacity_after_header() const 
	{
		return capacity_after_header_;
	}


	template<size_t length> void WriteBytesStatic(const void* data);

	template <typename T> bool WritePOD(const T& data) 
	{
		WriteBytesStatic<sizeof(data)>(&data);
		return true;
	}

	inline void WriteBytesCommon(const void* data, size_t length);

	void Resize(size_t new_capacity);

	static size_t AlignInt(size_t i, int alignment) 
	{
		return i + (alignment - (i % alignment)) % alignment;
	}

	static const int kPayloadUnit;

	Header* header_;
	size_t header_size_;
	size_t capacity_after_header_;
	size_t write_offset_;
};