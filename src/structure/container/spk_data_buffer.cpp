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

	void DataBuffer::resize(const size_t& p_newSize)
	{
		_data.resize(p_newSize);
	}

	void DataBuffer::skip(const size_t& p_bytesToSkip) const
	{
		if (leftover() < p_bytesToSkip)
			throw std::runtime_error(std::string("Unable to skip ") + std::to_string(p_bytesToSkip) + " bytes.");
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