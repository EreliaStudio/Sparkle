#include "structure/container/spk_data_buffer.hpp"

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

	uint8_t* DataBuffer::data()
	{
		return (_data.data());
	}

	const uint8_t* DataBuffer::data() const
	{
		return (_data.data());
	}

	size_t DataBuffer::size() const { return _data.size(); }

	size_t DataBuffer::bookmark() const { return _bookmark; }

	size_t DataBuffer::leftover() const { return size() - bookmark(); }

	bool DataBuffer::empty() const { return leftover() == 0; }

	void DataBuffer::resize(const size_t& p_newSize)
	{
		_data.resize(p_newSize);
	}

	void DataBuffer::skip(const size_t& p_number)
	{
		if (leftover() < p_number)
			throw std::runtime_error(std::string("Unable to skip ") + std::to_string(p_number) + " bytes.");
		_bookmark += p_number;
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

	void DataBuffer::edit(const size_t& p_offset, const void* p_data, const size_t& p_dataSize)
	{
		if (p_offset + p_dataSize > size())
			throw std::runtime_error("Unable to edit, offset is out of bound.");
		memcpy(_data.data() + p_offset, p_data, p_dataSize);
	}

	void DataBuffer::append(const void* p_data, const size_t& p_dataSize)
	{
		size_t oldSize = size();
		resize(size() + p_dataSize);
		edit(oldSize, p_data, p_dataSize);
	}
}