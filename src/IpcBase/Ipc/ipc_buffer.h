#pragma once

class IpcBuffer 
{
public:
	explicit IpcBuffer(int size) : size_(size), data_(NULL)
	{
		data_ = malloc(size_);
	}
	virtual ~IpcBuffer()
	{
		if (data_)
			free(data_);
	}

	// Returns the size of the Pickle's data.
	size_t size() const { return size_; }

	// Returns the data for this Pickle.
	void* data() const { return data_; }

private:
	size_t size_;
	void   *data_;
};