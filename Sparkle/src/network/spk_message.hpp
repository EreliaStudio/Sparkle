#pragma once

#include <cstdint>
#include <cstring>

#include "data_structure/spk_data_buffer.hpp"

namespace spk
{
    class Message
    {
    public:
        struct Header
        {
            size_t type;
            uint8_t reserved[16];
            size_t length;

            Header() : type(0), length(0)
            {
                std::memset(reserved, 0, sizeof(reserved));
            }
        };

    private:
        Header _header;
		DataBuffer _data;

    public:
        Message() = default;

        const Header& header() const
        {
            return (_header);
        }

		const DataBuffer& data() const
		{
			return (_data);
		}

		size_t size() const
		{
			return (_data.size());
		}

        void resize(const size_t& p_newSize)
        {
            _data.resize(p_newSize);
            _header.length = _data.size();
        }

        void clear()
        {
            _data.clear();
            _header.length = _data.size();
        }

        void reset()
        {
            _data.reset();
            _header.length = _data.size();
        }

        void skip(const size_t& p_number)
        {
            _data.skip(p_number);
        }

        template <typename InputType>
        void edit(const size_t& p_offset, const InputType& p_input)
        {
            _data.edit(p_offset, p_input);
        }

        void edit(const size_t& p_offset, const void* p_data, const size_t& p_dataSize)
        {
            _data.edit(p_offset, p_data, p_dataSize);
        }

        void append(const void* p_data, const size_t& p_dataSize)
        {
            _data.append(p_data, p_dataSize);
            _header.length = _data.size();
        }

        template <typename InputType>
        Message& operator<<(const InputType& p_input)
        {
            _data << p_input;
            _header.length = _data.size();
            return *this;
        }

        template <typename OutputType>
        const Message& operator>>(OutputType& p_output) const
        {
            _data >> p_output;
            return *this;
        }
    };
}
