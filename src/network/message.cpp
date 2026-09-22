#include "network/message.hpp"

#include "exception.hpp"

#include <cstring>
#include <limits>
#include <utility>

namespace spk
{
	Message::Message(Type type) noexcept :
		_type(type)
	{
	}

	Message::Message(Type type, Storage payload) noexcept :
		_type(type),
		_payload(std::move(payload))
	{
	}

	void Message::setType(Type type) noexcept
	{
		_type = type;
	}

	Message::Type Message::type() const noexcept
	{
		return _type;
	}

	void Message::clear() noexcept
	{
		_payload.clear();
		_readOffset = 0;
	}

	void Message::reset() const noexcept
	{
		_readOffset = 0;
	}

	void Message::resize(std::size_t size)
	{
		_payload.resize(size);
		if (_readOffset > size)
		{
			_readOffset = size;
		}
	}

	void Message::skip(std::size_t size) const
	{
		if (size > _payload.size() - _readOffset)
		{
			throw Exception("Unable to skip beyond the end of a network message.");
		}
		_readOffset += size;
	}

	void Message::edit(std::size_t offset, const void *data, std::size_t size)
	{
		if (offset > _payload.size() || size > _payload.size() - offset)
		{
			throw Exception("Unable to edit outside a network message payload.");
		}
		std::memcpy(_payload.data() + offset, data, size);
	}

	void Message::append(const void *data, std::size_t size)
	{
		const auto *bytes = static_cast<const std::byte *>(data);
		_payload.insert(_payload.end(), bytes, bytes + size);
	}

	void Message::push(const void *data, std::size_t size)
	{
		append(data, size);
	}

	void Message::pull(void *data, std::size_t size) const
	{
		if (size > _payload.size() - _readOffset)
		{
			throw Exception("Unable to read beyond the end of a network message.");
		}
		std::memcpy(data, _payload.data() + _readOffset, size);
		_readOffset += size;
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
		return _readOffset;
	}

	Message &Message::operator<<(std::string_view value)
	{
		if (value.size() > std::numeric_limits<std::uint32_t>::max())
		{
			throw Exception("String is too large to serialize into a network message.");
		}
		*this << static_cast<std::uint32_t>(value.size());
		append(value.data(), value.size());
		return *this;
	}

	const Message &Message::operator>>(std::string &value) const
	{
		const std::uint32_t size = get<std::uint32_t>();
		value.resize(size);
		pull(value.data(), size);
		return *this;
	}

	std::byte Message::_readByte() const
	{
		if (_readOffset >= _payload.size())
		{
			throw Exception("Unable to read beyond the end of a network message.");
		}
		return _payload[_readOffset++];
	}
}
