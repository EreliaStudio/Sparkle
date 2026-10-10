#include "network/message.hpp"

#include "exception.hpp"

#include <cstring>
#include <limits>
#include <utility>

namespace spk
{
	Message::Writer::Writer(Type type) noexcept
	{
		_header.messageType = type;
	}

	Message::Writer::Writer(Message &&message) :
		_header(std::exchange(message._header, Header{})),
		_payload(std::move(message._payload))
	{
	}

	Message::Header &Message::Writer::header() noexcept
	{
		return _header;
	}

	const Message::Header &Message::Writer::header() const noexcept
	{
		return _header;
	}

	spk::ByteStream::Writer &Message::Writer::payload() noexcept
	{
		return _payload;
	}

	const spk::ByteStream::Writer &Message::Writer::payload() const noexcept
	{
		return _payload;
	}

	void Message::Writer::setType(Type type) noexcept
	{
		_header.messageType = type;
	}

	Message::Type Message::Writer::type() const noexcept
	{
		return _header.messageType;
	}

	void Message::Writer::setRequestID(RequestID id) noexcept
	{
		_header.requestID = id;
	}

	Message::RequestID Message::Writer::requestID() const noexcept
	{
		return _header.requestID;
	}

	void Message::Writer::clear() noexcept
	{
		_payload.clear();
	}

	void Message::Writer::resize(std::size_t size)
	{
		_payload.resize(size);
	}

	void Message::Writer::edit(std::size_t offset, const void *data, std::size_t size)
	{
		_payload.edit(offset, data, size);
	}

	void Message::Writer::append(const void *data, std::size_t size)
	{
		_payload.append(data, size);
	}

	void Message::Writer::push(const void *data, std::size_t size)
	{
		_payload.append(data, size);
	}

	std::span<std::byte> Message::Writer::data() noexcept
	{
		return _payload.writableData();
	}

	std::span<const std::byte> Message::Writer::data() const noexcept
	{
		return _payload.data();
	}

	std::size_t Message::Writer::size() const noexcept
	{
		return _payload.size();
	}

	std::size_t Message::Writer::capacity() const noexcept
	{
		return _payload.capacity();
	}

	bool Message::Writer::empty() const noexcept
	{
		return _payload.size() == 0;
	}

	Message::Writer &Message::Writer::operator<<(std::string_view value)
	{
		_payload << value;
		return *this;
	}

	Message Message::Writer::build() &&
	{
		return Message(std::exchange(_header, Header{}), std::move(_payload).build());
	}

	Message::Message(Header header, spk::ByteStream payload) :
		_header(header),
		_payload(std::move(payload))
	{
	}

	const Message::Header &Message::header() const noexcept
	{
		return _header;
	}

	const spk::ByteStream &Message::payload() const noexcept
	{
		return _payload;
	}

	spk::ByteStream &Message::payload() noexcept
	{
		return _payload;
	}

	Message::Type Message::type() const noexcept
	{
		return _header.messageType;
	}

	Message::RequestID Message::requestID() const noexcept
	{
		return _header.requestID;
	}

	std::span<const std::byte> Message::data() const noexcept
	{
		return _payload.data();
	}

	std::size_t Message::size() const noexcept
	{
		return _payload.size();
	}

	bool Message::empty() const noexcept
	{
		return _payload.empty();
	}

	Message::Reader Message::reader(std::size_t offset) const
	{
		auto result = _payload.reader();
		result.seek(offset);
		return result;
	}
}
