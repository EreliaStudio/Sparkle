#pragma once

#include "structure/container/spk_data_buffer.hpp"
#include <array>
#include <cstdint>
#include <cstring>

namespace spk
{
	class Server;
	class Client;

	class Message
	{
		friend class Server;
		friend class Client;

	public:
		class Header
		{
		public:
			using ClientId = long;
			static inline const ClientId InvalidID = -1;
			using Type = int32_t;

		private:
			friend class Message;

		public:
			Type type;
			size_t length;
			ClientId emitterID;
			std::array<uint8_t, 16> reserved;

			Header();
			Header(Type p_type);
			Header(const ClientId &p_emitterID, const Type &p_type);
		};

	private:
		Header _header;
		DataBuffer _buffer;

	public:
		Message();
		Message(const Header::Type &p_messageType);
		Message(const Header::ClientId &p_clientToRedirectMessage, const Header::Type &p_messageType);

		void setType(const Header::Type &p_type);
		void setEmitterID(const Header::ClientId &p_emitterID);
		void resize(const size_t &p_newSize);
		void clear();
		void reset();
		void skip(const size_t &p_bytesToSkip) const;
		void edit(const size_t &p_offset, const void *p_data, const size_t &p_dataSize);
		void append(const void *p_data, const size_t &p_dataSize);

		template <typename TType>
		void skip() const
		{
			skip(sizeof(TType));
		}

		template <typename InputType>
		void edit(const size_t &p_offset, const InputType &p_input)
		{
			_buffer.edit(p_offset, p_input);
		}

		void push(const void *p_buffer, size_t p_nbBytes);
		void pull(void *p_buffer, size_t p_nbBytes) const;

		template <typename InputType>
		Message &operator<<(const InputType &p_input)
		{
			_buffer << p_input;
			_header.length = _buffer.size();
			return *this;
		}

		template <typename OutputType>
		const Message &operator>>(OutputType &p_output) const
		{
			_buffer >> p_output;
			return *this;
		}

		template <typename OutputType>
		OutputType get() const
		{
			return (_buffer.get<OutputType>());
		}

		template <typename OutputType>
		OutputType peek() const
		{
			return (_buffer.peek<OutputType>());
		}

		const Header &header() const;
		const DataBuffer &buffer() const;
		size_t size() const;
		bool empty() const;
	};
}
