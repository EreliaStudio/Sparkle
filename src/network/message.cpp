#include "network/message.hpp"

#include "exception.hpp"

#include <cstring>
#include <limits>
#include <utility>

namespace spk
{
	Message::Reader::Reader(spk::ByteStream payload, std::size_t offset) :
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

	Message::Writer::Writer(Type type) noexcept :
		_type(type)
	{
	}

	Message::Writer::Writer(Message &&message) :
		_type(std::exchange(message._type, 0)),
		_requestID(std::exchange(message._requestID, 0))
	{
		if (message._storage == nullptr)
		{
			return;
		}

		if (message._storage.use_count() == 1)
		{
			_storage = std::move(*message._storage);
			message._storage.reset();
			return;
		}

		const auto source = message.data();
		_ensureCapacity(source.size());
		if (source.empty() == false)
		{
			_storage->resize(source.size());
			std::memcpy(
				_storage->data(),
				source.data(),
				source.size());
		}
		message._storage.reset();
	}

	void Message::Writer::_ensureCapacity(std::size_t requiredCapacity)
	{
		if (requiredCapacity == 0)
		{
			return;
		}

		if (
			static_cast<bool>(_storage) == true &&
			_storage->capacity() >= requiredCapacity)
		{
			return;
		}

		Storage::Lease replacement = Message::_obtainStorage(requiredCapacity);

		if (static_cast<bool>(_storage) == true)
		{
			replacement->resize(_storage->size());
			if (_storage->empty() == false)
			{
				std::memcpy(
					replacement->data(),
					_storage->data(),
					_storage->size());
			}
		}

		_storage = std::move(replacement);
	}

	void Message::Writer::setType(Type type) noexcept
	{
		_type = type;
	}

	Message::Type Message::Writer::type() const noexcept
	{
		return _type;
	}

	void Message::Writer::setRequestID(RequestID requestID) noexcept
	{
		_requestID = requestID;
	}

	Message::RequestID Message::Writer::requestID() const noexcept
	{
		return _requestID;
	}

	void Message::Writer::clear() noexcept
	{
		if (static_cast<bool>(_storage) == true)
		{
			_storage->clear();
		}
	}

	void Message::Writer::resize(std::size_t size)
	{
		_ensureCapacity(size);
		if (static_cast<bool>(_storage) == true)
		{
			_storage->resize(size);
		}
	}

	void Message::Writer::edit(
		std::size_t offset,
		const void *data,
		std::size_t size)
	{
		if (offset > this->size() || size > this->size() - offset)
		{
			throw Exception("Unable to edit outside a network message payload.");
		}
		if (size != 0)
		{
			std::memcpy(_storage->data() + offset, data, size);
		}
	}

	void Message::Writer::append(const void *data, std::size_t size)
	{
		if (size == 0)
		{
			return;
		}

		const std::size_t currentSize = this->size();
		if (size > std::numeric_limits<std::size_t>::max() - currentSize)
		{
			throw Exception("Network message payload size overflow.");
		}

		const std::size_t requiredSize = currentSize + size;
		_ensureCapacity(requiredSize);
		_storage->resize(requiredSize);
		std::memcpy(_storage->data() + currentSize, data, size);
	}

	void Message::Writer::push(const void *data, std::size_t size)
	{
		append(data, size);
	}

	std::span<std::byte> Message::Writer::data() noexcept
	{
		if (static_cast<bool>(_storage) == false)
		{
			return {};
		}

		return std::span<std::byte>(_storage->data(), _storage->size());
	}

	std::span<const std::byte> Message::Writer::data() const noexcept
	{
		if (static_cast<bool>(_storage) == false)
		{
			return {};
		}

		return std::span<const std::byte>(_storage->data(), _storage->size());
	}

	std::size_t Message::Writer::size() const noexcept
	{
		if (static_cast<bool>(_storage) == false)
		{
			return 0;
		}

		return _storage->size();
	}

	std::size_t Message::Writer::capacity() const noexcept
	{
		if (static_cast<bool>(_storage) == false)
		{
			return 0;
		}

		return _storage->capacity();
	}

	bool Message::Writer::empty() const noexcept
	{
		return size() == 0;
	}

	Message::Writer &Message::Writer::operator<<(std::string_view value)
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

	Message Message::Writer::build() &&
	{
		return Message(
			std::exchange(_type, 0),
			std::exchange(_requestID, 0),
			std::move(_storage));
	}

	Message::Message(
		Type type,
		RequestID requestID,
		Storage::Lease storage) :
		_type(type),
		_requestID(requestID)
	{
		if (static_cast<bool>(storage) == true)
		{
			_storage =
				std::make_shared<Storage::Lease>(
					std::move(storage));
		}
	}

	Message::Type Message::type() const noexcept
	{
		return _type;
	}

	Message::RequestID Message::requestID() const noexcept
	{
		return _requestID;
	}

	std::span<const std::byte> Message::data() const noexcept
	{
		if (_storage == nullptr || static_cast<bool>(*_storage) == false)
		{
			return {};
		}

		const Storage &storage = **_storage;
		return std::span<const std::byte>(storage.data(), storage.size());
	}

	std::size_t Message::size() const noexcept
	{
		return data().size();
	}

	bool Message::empty() const noexcept
	{
		return size() == 0;
	}

	spk::ByteStream Message::payload() const
	{
		if (_storage == nullptr || static_cast<bool>(*_storage) == false)
		{
			return spk::ByteStream{};
		}
		const auto *buffer = static_cast<const spk::ByteStream::Buffer *>(&(**_storage));
		return spk::ByteStream::share(std::shared_ptr<const spk::ByteStream::Buffer>(_storage, buffer));
	}

	Message::Reader Message::reader(std::size_t offset) const
	{
		return Reader(payload(), offset);
	}
}
