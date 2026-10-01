#include "network/message.hpp"

#include "exception.hpp"

#include <cstring>
#include <limits>
#include <utility>

namespace spk
{
	Message::Reader::Reader(
		const Message &message,
		std::size_t offset) :
		_message(&message)
	{
		_seek(offset);
	}

	void Message::Reader::_seek(std::size_t offset) const
	{
		if (offset > _message->size())
		{
			throw Exception("Unable to seek beyond the end of a network message.");
		}
		_readOffset = offset;
	}

	void Message::Reader::reset() const noexcept
	{
		_readOffset = 0;
	}

	void Message::Reader::seek(std::size_t offset) const
	{
		_seek(offset);
	}

	void Message::Reader::skip(std::size_t size) const
	{
		if (
			_readOffset > _message->size() ||
			size > _message->size() - _readOffset)
		{
			throw Exception("Unable to skip beyond the end of a network message.");
		}
		_readOffset += size;
	}

	void Message::Reader::pull(void *data, std::size_t size) const
	{
		_message->readAt(_readOffset, data, size);
		_readOffset += size;
	}

	std::size_t Message::Reader::readOffset() const noexcept
	{
		return _readOffset;
	}

	const Message::Reader &Message::Reader::operator>>(std::string &value) const
	{
		const std::uint32_t size = get<std::uint32_t>();
		value.resize(size);
		pull(value.data(), size);
		return *this;
	}

	Message::Message() noexcept :
		_reader(*this)
	{
	}

	Message::Message(Type type) noexcept :
		_type(type),
		_reader(*this)
	{
	}

	Message::Message(Type type, Storage payload) noexcept :
		_type(type),
		_payload(std::move(payload)),
		_reader(*this)
	{
	}

	Message::Message(const Message &other) :
		_type(other._type),
		_requestID(other._requestID),
		_payload(other._payload),
		_reader(*this, other.readOffset())
	{
	}

	Message::Message(Message &&other) noexcept :
		_type(other._type),
		_requestID(other._requestID),
		_payload(std::move(other._payload)),
		_reader(*this, other.readOffset())
	{
		other._reader.reset();
	}

	Message &Message::operator=(const Message &other)
	{
		if (this == &other)
		{
			return *this;
		}

		_type = other._type;
		_requestID = other._requestID;
		_payload = other._payload;
		_reader.seek(other.readOffset());
		return *this;
	}

	Message &Message::operator=(Message &&other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		const std::size_t offset = other.readOffset();

		_type = other._type;
		_requestID = other._requestID;
		_payload = std::move(other._payload);
		_reader.seek(offset);
		other._reader.reset();
		return *this;
	}

	void Message::setType(Type type) noexcept
	{
		_type = type;
	}

	Message::Type Message::type() const noexcept
	{
		return _type;
	}

	void Message::setRequestID(RequestID requestID) noexcept
	{
		_requestID = requestID;
	}

	Message::RequestID Message::requestID() const noexcept
	{
		return _requestID;
	}

	void Message::clear() noexcept
	{
		_payload.clear();
		_reader.reset();
	}

	void Message::reset() const noexcept
	{
		_reader.reset();
	}

	void Message::resize(std::size_t size)
	{
		_payload.resize(size);
		if (_reader.readOffset() > size)
		{
			_reader.seek(size);
		}
	}

	void Message::skip(std::size_t size) const
	{
		_reader.skip(size);
	}

	void Message::edit(std::size_t offset, const void *data, std::size_t size)
	{
		if (offset > _payload.size() || size > _payload.size() - offset)
		{
			throw Exception("Unable to edit outside a network message payload.");
		}
		if (size != 0)
		{
			std::memcpy(_payload.data() + offset, data, size);
		}
	}

	void Message::readAt(std::size_t offset, void *destination, std::size_t size) const
	{
		if (offset > _payload.size() || size > _payload.size() - offset)
		{
			throw Exception("Unable to read outside a network message payload.");
		}
		if (size != 0)
		{
			std::memcpy(destination, _payload.data() + offset, size);
		}
	}

	void Message::append(const void *data, std::size_t size)
	{
		if (size == 0)
		{
			return;
		}
		const auto *bytes = static_cast<const std::byte *>(data);
		_payload.insert(_payload.end(), bytes, bytes + size);
	}

	void Message::push(const void *data, std::size_t size)
	{
		append(data, size);
	}

	void Message::pull(void *data, std::size_t size) const
	{
		_reader.pull(data, size);
	}

	std::span<const std::byte> Message::data() const noexcept
	{
		return _payload;
	}

	std::size_t Message::size() const noexcept
	{
		return _payload.size();
	}

	bool Message::empty() const noexcept
	{
		return _payload.empty();
	}

	std::size_t Message::readOffset() const noexcept
	{
		return _reader.readOffset();
	}

	Message &Message::operator<<(std::string_view value)
	{
		if (value.size() > std::numeric_limits<std::uint32_t>::max())
		{
			throw Exception("String is too large to serialize into a network message.");
		}

		const auto size = static_cast<std::uint32_t>(value.size());
		append(size);
		append(value.data(), value.size());
		return *this;
	}

	const Message &Message::operator>>(std::string &value) const
	{
		_reader >> value;
		return *this;
	}

	Message::Reader Message::reader(std::size_t offset) const
	{
		return Reader(*this, offset);
	}
}
