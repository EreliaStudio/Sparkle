#pragma once

#include <cstdint>
#include <cstring>

#include "data_structure/spk_data_buffer.hpp"

namespace spk
{
    class Message
    {
    public:
        class Header
        {
        public:
            using ClientID = size_t;
            using Type = int32_t;
    
        private:
            friend class Message;
            size_t _length;

        public:
            int32_t type;
            ClientID emitterID;
            uint8_t reserved[16];

            Header() :
                type(0),
                emitterID(0),
                _length(0)
            {
                std::memset(reserved, 0, sizeof(reserved));
            }

            Header(Type p_type) :
                type(p_type),
                emitterID(0),
                _length(0)
            {
                std::memset(reserved, 0, sizeof(reserved));
            }

            Header(const ClientID& p_emitterID, const Type& p_type) :
                type(p_type),
                emitterID(p_emitterID),
                _length(0)
            {
                std::memset(reserved, 0, sizeof(reserved));
            }
            
            size_t length() const
            {
                return (_length);
            }
        };

    private:
        Header _header;
		DataBuffer _buffer;

    public:
        Message() = default;

        Message(const Header::Type& p_messageType) :
            _header(p_messageType)
        {

        }

        Message(const Header::ClientID& p_clientToRedirectMessage, const Header::Type& p_messageType) :
            _header(p_clientToRedirectMessage, p_messageType)
        {

        }

        const Header::Type& type() const
        {
            return (_header.type);
        }

        Header& header()
        {
            return (_header);
        }

        const Header& header() const
        {
            return (_header);
        }

		DataBuffer& buffer()
		{
			return (_buffer);
		}

		const DataBuffer& buffer() const
		{
			return (_buffer);
		}

        const uint8_t* data() const
        {
            return (_buffer.data());
        }

		size_t size() const
		{
			return (_buffer.size());
		}

        void resize(const size_t& p_newSize)
        {
            _buffer.resize(p_newSize);
            _header._length = _buffer.size();
        }

        void clear()
        {
            _buffer.clear();
            _header._length = _buffer.size();
        }

        void reset()
        {
            _buffer.reset();
            _header._length = _buffer.size();
        }

        void skip(const size_t& p_number)
        {
            _buffer.skip(p_number);
        }

        template <typename InputType>
        void edit(const size_t& p_offset, const InputType& p_input)
        {
            _buffer.edit(p_offset, p_input);
        }

        void edit(const size_t& p_offset, const void* p_data, const size_t& p_dataSize)
        {
            _buffer.edit(p_offset, p_data, p_dataSize);
        }

        void append(const void* p_data, const size_t& p_dataSize)
        {
            _buffer.append(p_data, p_dataSize);
            _header._length = _buffer.size();
        }

		template <typename OutputType>
		OutputType get() const
        {
            return (_buffer.get<OutputType>());    
        }

        template <typename InputType>
        Message& operator<<(const InputType& p_input)
        {
            _buffer << p_input;
            _header._length = _buffer.size();
            return *this;
        }

        template <typename OutputType>
        const Message& operator>>(OutputType& p_output) const
        {
            _buffer >> p_output;
            return *this;
        }
    };
}
