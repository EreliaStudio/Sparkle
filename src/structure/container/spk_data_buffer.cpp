#include "structure/container/spk_data_buffer.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	DataBuffer::DataBuffer() :
		_data(),
		_bookmark(0)
	{
	}

	DataBuffer::DataBuffer(size_t p_dataSize) :
		_data(p_dataSize),
		_bookmark(0)
	{
	}

	DataBuffer::DataBuffer(const DataBuffer &p_other) :
		_data(p_other._data),
		_bookmark(p_other._bookmark)
	{
	}

	DataBuffer &DataBuffer::operator=(const DataBuffer &p_other)
	{
		if (this != &p_other)
		{
			_data = p_other._data;
			_bookmark = p_other._bookmark;
		}
		return *this;
	}

	DataBuffer::DataBuffer(DataBuffer &&p_other) :
		_data(std::move(p_other._data)),
		_bookmark(p_other._bookmark)
	{
		p_other._bookmark = 0;
	}

	DataBuffer &DataBuffer::operator=(DataBuffer &&p_other)
	{
		if (this != &p_other)
		{
			_data = std::move(p_other._data);
			_bookmark = p_other._bookmark;
			p_other._bookmark = 0;
		}
		return *this;
	}

	uint8_t *DataBuffer::data()
	{
		return (_data.data());
	}

	const uint8_t *DataBuffer::data() const
	{
		return (_data.data());
	}

	size_t DataBuffer::size() const
	{
		return _data.size();
	}

	size_t DataBuffer::bookmark() const
	{
		return _bookmark;
	}

	size_t DataBuffer::leftover() const
	{
		return size() - bookmark();
	}

	bool DataBuffer::empty() const
	{
		return leftover() == 0;
	}

	void DataBuffer::resize(const size_t &p_newSize)
	{
		_data.resize(p_newSize);
	}

	void DataBuffer::skip(const size_t &p_bytesToSkip) const
	{
		if (leftover() < p_bytesToSkip)
		{
			GENERATE_ERROR(std::string("Unable to skip ") + std::to_string(p_bytesToSkip) + " bytes.");
		}
		_bookmark += p_bytesToSkip;
	}

	void DataBuffer::clear()
	{
		_data.clear();
		_bookmark = 0;
	}

	void DataBuffer::reset()
	{
		_bookmark = 0;
	}
}