#include "pickle.h"
#include <stdlib.h>

const int Pickle::kPayloadUnit = 64;

static const size_t kCapacityReadOnly = static_cast<size_t>(-1);

PickleIterator::PickleIterator(const Pickle& pickle)
	: read_ptr_(pickle.payload())
	, read_end_ptr_(pickle.end_of_payload()) 
{
}

bool PickleIterator::ReadBool(bool* result) {
	return ReadBuiltinType(result);
}

bool PickleIterator::ReadInt(int* result) {
	return ReadBuiltinType(result);
}

bool PickleIterator::ReadLong(long* result) {
	return ReadBuiltinType(result);
}

bool PickleIterator::ReadUInt16(uint16* result) {
	return ReadBuiltinType(result);
}

bool PickleIterator::ReadUInt32(uint32* result) {
	return ReadBuiltinType(result);
}

bool PickleIterator::ReadInt64(int64* result) {
	return ReadBuiltinType(result);
}

bool PickleIterator::ReadUInt64(uint64* result) {
	return ReadBuiltinType(result);
}

bool PickleIterator::ReadFloat(float* result) {
	// crbug.com/315213
	// The source data may not be properly aligned, and unaligned float reads
	// cause SIGBUS on some ARM platforms, so force using memcpy to copy the data
	// into the result.
	const char* read_from = GetReadPointerAndAdvance<float>();
	if (!read_from)
		return false;
	memcpy(result, read_from, sizeof(*result));
	return true;
}

bool PickleIterator::ReadString(std::string* result) {
	int len;
	if (!ReadInt(&len))
		return false;
	const char* read_from = GetReadPointerAndAdvance(len);
	if (!read_from)
		return false;

	result->assign(read_from, len);
	return true;
}

bool PickleIterator::ReadWString(std::wstring* result) {
	int len;
	if (!ReadInt(&len))
		return false;
	const char* read_from = GetReadPointerAndAdvance(len, sizeof(wchar_t));
	if (!read_from)
		return false;

	result->assign(reinterpret_cast<const wchar_t*>(read_from), len);
	return true;
}

bool PickleIterator::ReadData(const char** data, int* length) {
	*length = 0;
	*data = 0;

	if (!ReadInt(length))
		return false;

	return ReadBytes(data, *length);
}

bool PickleIterator::ReadBytes(const char** data, int length) {
	const char* read_from = GetReadPointerAndAdvance(length);
	if (!read_from)
		return false;
	*data = read_from;
	return true;
}

template <typename Type>
inline bool PickleIterator::ReadBuiltinType(Type* result) 
{
	const char* read_from = GetReadPointerAndAdvance<Type>();
	if (!read_from)
		return false;
	if (sizeof(Type) > sizeof(unsigned int))
		memcpy(result, read_from, sizeof(*result));
	else
		*result = *reinterpret_cast<const Type*>(read_from);
	return true;
}

template<typename Type>
inline const char* PickleIterator::GetReadPointerAndAdvance() 
{
	const char* current_read_ptr = read_ptr_;
	if (read_ptr_ + sizeof(Type) > read_end_ptr_)
		return NULL;
	if (sizeof(Type) < sizeof(unsigned int))
		read_ptr_ += AlignInt(sizeof(Type), sizeof(unsigned int));
	else
		read_ptr_ += sizeof(Type);
	return current_read_ptr;
}

const char* PickleIterator::GetReadPointerAndAdvance(int num_bytes) 
{
	if (num_bytes < 0 || read_end_ptr_ - read_ptr_ < num_bytes)
		return NULL;
	const char* current_read_ptr = read_ptr_;
	read_ptr_ += AlignInt(num_bytes, sizeof(unsigned int));
	return current_read_ptr;
}

inline const char* PickleIterator::GetReadPointerAndAdvance(int num_elements,
															size_t size_element) 
{
	// Check for int32 overflow.
	long long num_bytes = static_cast<long long>(num_elements) * size_element;
	int num_bytes32 = static_cast<int>(num_bytes);
	if (num_bytes != static_cast<long long>(num_bytes32))
		return NULL;
	return GetReadPointerAndAdvance(num_bytes32);
}

Pickle::Pickle(int header_size)
	: header_(NULL)
	, header_size_(AlignInt(header_size, sizeof(unsigned int)))
	, capacity_after_header_(0)
	, write_offset_(0)
{
	Resize(kPayloadUnit);
	header_->payload_size = 0;
}

Pickle::Pickle(const char* data, int data_len)
	: header_(reinterpret_cast<Header*>(const_cast<char*>(data)))
	, header_size_(0)
	, capacity_after_header_(kCapacityReadOnly)
	, write_offset_(0) 
{
	if (data_len >= static_cast<int>(sizeof(Header)))
		header_size_ = data_len - header_->payload_size;

	if (header_size_ > static_cast<unsigned int>(data_len))
		header_size_ = 0;

	if (header_size_ != AlignInt(header_size_, sizeof(unsigned int)))
		header_size_ = 0;

	// If there is anything wrong with the data, we're not going to use it.
	if (!header_size_)
		header_ = NULL;
}

Pickle::~Pickle()
{
	if (capacity_after_header_ != kCapacityReadOnly)
		free(header_);
}

Pickle::Pickle(const Pickle& other)
	: header_(NULL)
	, header_size_(other.header_size_)
	, capacity_after_header_(0)
	, write_offset_(other.write_offset_) 
{
	size_t payload_size = header_size_ + other.header_->payload_size;
	Resize(payload_size);
	memcpy(header_, other.header_, payload_size);
}

Pickle& Pickle::operator=(const Pickle& other) 
{
	if (this == &other) 
	{
		return *this;
	}
	if (capacity_after_header_ == kCapacityReadOnly) 
	{
		header_ = NULL;
		capacity_after_header_ = 0;
	}
	if (header_size_ != other.header_size_) {
		free(header_);
		header_ = NULL;
		header_size_ = other.header_size_;
	}
	Resize(other.header_->payload_size);
	memcpy(header_, other.header_,
		other.header_size_ + other.header_->payload_size);
	write_offset_ = other.write_offset_;
	return *this;
}

bool Pickle::WriteString(const std::string& value) 
{
	if (!WriteInt(static_cast<int>(value.size())))
		return false;

	return WriteBytes(value.data(), static_cast<int>(value.size()));
}

bool Pickle::WriteWString(const std::wstring& value) 
{
	if (!WriteInt(static_cast<int>(value.size())))
		return false;

	return WriteBytes(value.data(),
		static_cast<int>(value.size() * sizeof(wchar_t)));
}

bool Pickle::WriteData(const char* data, int length) 
{
	return length >= 0 && WriteInt(length) && WriteBytes(data, length);
}

bool Pickle::WriteBytes(const void* data, int length) 
{
	WriteBytesCommon(data, length);
	return true;
}


template <size_t length> void Pickle::WriteBytesStatic(const void* data) 
{
	WriteBytesCommon(data, length);
}

template void Pickle::WriteBytesStatic<2>(const void* data);
template void Pickle::WriteBytesStatic<4>(const void* data);
template void Pickle::WriteBytesStatic<8>(const void* data);

inline void Pickle::WriteBytesCommon(const void* data, size_t length) 
{
	size_t data_len = AlignInt(length, sizeof(uint32));
	size_t new_size = write_offset_ + data_len;
	if (new_size > capacity_after_header_) 
	{
		size_t max_size = (capacity_after_header_ * 2 > new_size) ? capacity_after_header_ * 2 : new_size;
		Resize(max_size);
	}

	char* write = mutable_payload() + write_offset_;
	memcpy(write, data, length);
	memset(write + length, 0, data_len - length);
	header_->payload_size = static_cast<uint32>(write_offset_ + length);
	write_offset_ = new_size;
}


void Pickle::Resize(size_t new_capacity) 
{
	new_capacity = AlignInt(new_capacity, kPayloadUnit);
	void* p = realloc(header_, header_size_ + new_capacity);
	header_ = reinterpret_cast<Header*>(p);
	capacity_after_header_ = new_capacity;
}

// static
const char* Pickle::FindNext(size_t header_size,
							 const char* start,
							 const char* end) 
{
	size_t length = static_cast<size_t>(end - start);
	if (length < sizeof(Header))
		return NULL;

	const Header* hdr = reinterpret_cast<const Header*>(start);
	if (length < header_size || length - header_size < hdr->payload_size)
		return NULL;
	return start + header_size + hdr->payload_size;
}

