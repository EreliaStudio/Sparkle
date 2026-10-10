#include "network/message.hpp"

#include "exception.hpp"

#include <cstring>
#include <limits>
#include <utility>

namespace spk
{
	Message::Reader::Reader(const spk::ByteStream &payload, std::size_t offset) :
		_slice(payload.reader())
	{
		_slice.seek(offset);
	}

	void Message::Reader::reset() const noexcept
	{
		_slice.reset();
	}

	void Message::Reader::seek(std::size_t offset) const
	{
		_slice.seek(offset);
	}

	void Message::Reader::skip(std::size_t count) const
	{
		_slice.skip(count);
	}

	void Message::Reader::pull(void *destination, std::size_t count) const
	{
		_slice.pull(destination, count);
	}

	void Message::Reader::readAt(std::size_t offset, void *destination, std::size_t count) const
	{
		if (offset > _slice.size() || count > _slice.size() - offset)
		{
			throw spk::Exception("Unable to read outside a network message payload.");
		}
		_slice.slice(offset, offset + count).pull(destination, count);
	}

	std::size_t Message::Reader::readOffset() const noexcept
	{
		return _slice.readOffset();
	}

	std::span<const std::byte> Message::Reader::data() const noexcept
	{
		return _slice.data();
	}

	std::size_t Message::Reader::size() const noexcept
	{
		return _slice.size();
	}

	bool Message::Reader::empty() const noexcept
	{
		return _slice.empty();
	}

	const Message::Reader &Message::Reader::operator>>(std::string &value) const
	{
		_slice >> value;
		return *this;
	}

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
		return Reader(_payload, offset);
	}
}
