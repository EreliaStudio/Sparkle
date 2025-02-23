#include "structure/network/spk_message.hpp"
#include <cstring>

namespace spk
{
	Message::Header::Header() :
		type(0),
		emitterID(0),
		length(0)
	{
		std::memset(reserved, 0, sizeof(reserved));
	}

	Message::Header::Header(Type p_type) :
		type(p_type),
		emitterID(0),
		length(0)
	{
		std::memset(reserved, 0, sizeof(reserved));
	}

	Message::Header::Header(const ClientID &p_emitterID, const Type &p_type) :
		type(p_type),
		emitterID(p_emitterID),
		length(0)
	{
		std::memset(reserved, 0, sizeof(reserved));
	}

	Message::Message() :
		_header()
	{
		
	}

	Message::Message(const Header::Type &p_messageType) :
		_header(p_messageType)
	{

	}

	Message::Message(const Header::ClientID &p_clientToRedirectMessage, const Header::Type &p_messageType) :
		_header(p_clientToRedirectMessage, p_messageType)
	{

	}

	void Message::setType(const Header::Type &p_type)
	{
		_header.type = p_type;
	}

	void Message::setEmitterID(const Header::ClientID &p_emitterID)
	{
		_header.emitterID = p_emitterID;
	}

	void Message::resize(const size_t &p_newSize)
	{
		_buffer.resize(p_newSize);
		_header.length = _buffer.size();
	}

	void Message::clear()
	{
		_buffer.clear();
		_header.length = _buffer.size();
	}

	void Message::reset()
	{
		_buffer.reset();
		_header.length = _buffer.size();
	}

	void Message::skip(const size_t &p_number)
	{
		_buffer.skip(p_number);
	}

	void Message::edit(const size_t &p_offset, const void *p_data, const size_t &p_dataSize)
	{
		_buffer.edit(p_offset, p_data, p_dataSize);
	}

	void Message::append(const void *p_data, const size_t &p_dataSize)
	{
		_buffer.append(p_data, p_dataSize);
		_header.length = _buffer.size();
	}

	const Message::Header &Message::header() const
	{
		return _header;
	}

	const DataBuffer &Message::buffer() const
	{
		return _buffer;
	}

	size_t Message::size() const
	{
		return (sizeof(Header) + _buffer.size());
	}

	bool Message::empty() const
	{
		return (_buffer.empty());
	}
}
