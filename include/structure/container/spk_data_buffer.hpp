#pragma once

#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "spk_sfinae.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	class DataBuffer
	{
	private:
		std::vector<uint8_t> _data;
		mutable size_t _bookmark = 0;

	public:
		DataBuffer() = default;

		DataBuffer(size_t p_dataSize);

		DataBuffer(const DataBuffer &p_other) = default;
		DataBuffer &operator=(const DataBuffer &p_other) = default;

		DataBuffer(DataBuffer &&p_other) = default;
		DataBuffer &operator=(DataBuffer &&p_other) = default;

		uint8_t *data();
		const uint8_t *data() const;

		size_t size() const;

		size_t bookmark() const;

		size_t leftover() const;

		bool empty() const;

		void resize(const size_t &p_newSize);

		void skip(const size_t &p_bytesToSkip) const;

		void clear();

		void reset();

		void push(const void *p_buffer, size_t p_nbBytes);

		void pull(void *p_buffer, size_t p_nbBytes) const;

		template <typename OutputType, typename std::enable_if_t<!spk::IsContainer<OutputType>::value> * = nullptr>
		OutputType get() const
		{
			const OutputType &result = *(reinterpret_cast<const OutputType *>(_data.data() + bookmark()));
			skip(sizeof(OutputType));
			return (result);
		}

		template <typename OutputType, typename std::enable_if_t<!spk::IsContainer<OutputType>::value> * = nullptr>
		OutputType peek() const
		{
			const OutputType &result = *(reinterpret_cast<const OutputType *>(_data.data() + bookmark()));
			return (result);
		}

		template <typename OutputType, typename std::enable_if_t<spk::IsContainer<OutputType>::value> * = nullptr>
		OutputType get() const
		{
			OutputType result;
			*this >> result;
			return result;
		}

		template <typename OutputType, typename std::enable_if_t<spk::IsContainer<OutputType>::value> * = nullptr>
		OutputType peek() const
		{
			const size_t savedBookmark = bookmark();
			OutputType result;
			*this >> result;
			_bookmark = savedBookmark;
			return result;
		}

		template <typename InputType>
		void edit(const size_t &p_offset, const InputType &p_input)
		{
			static_assert(std::is_standard_layout<InputType>::value, "Unable to handle this type.");
			if (p_offset + sizeof(InputType) > size())
			{
				GENERATE_ERROR("Unable to edit, offset is out of bound.");
			}
			memcpy(_data.data() + p_offset, &p_input, sizeof(InputType));
		}

		void edit(const size_t &p_offset, const void *p_data, const size_t &p_dataSize);

		template <typename InputType, typename std::enable_if_t<!spk::IsContainer<InputType>::value> * = nullptr>
		DataBuffer &operator<<(const InputType &p_input)
		{
			static_assert(std::is_standard_layout<InputType>::value, "Unable to handle this type.");

			size_t previous_size(_data.size());

			_data.resize(_data.size() + sizeof(InputType));
			std::memcpy(_data.data() + previous_size, &p_input, sizeof(InputType));

			return *this;
		}

		template <typename OutputType, typename std::enable_if_t<!spk::IsContainer<OutputType>::value> * = nullptr>
		const DataBuffer &operator>>(OutputType &p_output) const
		{
			static_assert(std::is_standard_layout<OutputType>::value, "Unable to handle this type.");
			if (leftover() < sizeof(OutputType))
			{
				GENERATE_ERROR("Unable to retrieve data buffer content.");
			}
			std::memcpy(&p_output, _data.data() + bookmark(), sizeof(OutputType));
			_bookmark += sizeof(OutputType);
			return *this;
		}

		template <typename InputType, typename std::enable_if_t<spk::IsContainer<InputType>::value> * = nullptr>
		DataBuffer &operator<<(const InputType &p_input)
		{
			*this << p_input.size();
			for (auto it = p_input.begin(); it != p_input.end(); ++it)
			{
				*this << *it;
			}
			return *this;
		}

		template <typename OutputType, typename std::enable_if_t<spk::IsContainer<OutputType>::value> * = nullptr>
		const DataBuffer &operator>>(OutputType &p_output) const
		{
			p_output.resize(this->get<size_t>());
			for (auto it = p_output.begin(); it != p_output.end(); ++it)
			{
				*this >> *it;
			}
			return *this;
		}

		void append(const void *p_data, const size_t &p_dataSize);
	};
}
