#include "container/buffer.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <utility>

namespace spk
{
	CapacityPool<Buffer::Data> Buffer::Builder::_pool(
		[](std::size_t capacity) {
			auto data = std::make_unique<Data>();
			data->reserve(capacity);
			return data.release();
		},
		[](const Data &data) {
			return data.capacity();
		});

	std::byte *Buffer::Data::data()
	{
		return _storage.data();
	}

	const std::byte *Buffer::Data::data() const
	{
		return _storage.data();
	}

	std::size_t Buffer::Data::size() const
	{
		return _storage.size();
	}

	std::size_t Buffer::Data::capacity() const
	{
		return _storage.capacity();
	}

	std::uint64_t Buffer::Data::version() const
	{
		return _version;
	}

	void Buffer::Data::reserve(std::size_t capacity)
	{
		_storage.reserve(capacity);
	}

	void Buffer::Data::resize(std::size_t size)
	{
		_storage.resize(size);
	}

	void Buffer::Data::clear()
	{
		_storage.clear();
	}

	void Buffer::Data::reset()
	{
		clear();
		_version = 0;
	}

	void Buffer::Data::append(std::span<const std::byte> bytes)
	{
		if (bytes.empty())
		{
			return;
		}
		const auto previousSize = size();
		if (bytes.size() > _storage.max_size() - previousSize)
		{
			throw spk::Exception("Buffer append exceeds storage limits");
		}
		const auto *source = bytes.data();
		const auto *first = data();
		const bool internal = previousSize != 0 && !std::less<>{}(source, first) && std::less<>{}(source, first + previousSize);
		const auto offset = internal ? static_cast<std::size_t>(source - first) : 0;
		resize(previousSize + bytes.size());
		std::memmove(data() + previousSize, internal ? data() + offset : source, bytes.size());
	}

	void Buffer::Data::validate()
	{
		if (++_version == 0)
		{
			_version = 1;
		}
	}

	const std::byte *Buffer::data() const
	{
		return _getData().data();
	}

	std::size_t Buffer::size() const
	{
		return _getData().size();
	}

	std::uint64_t Buffer::version() const
	{
		return _getData().version();
	}

	bool Buffer::empty() const
	{
		return size() == 0;
	}

	Buffer::Immutable::Immutable(std::shared_ptr<const Data> data) :
		_data(std::move(data))
	{
	}

	const Buffer::Data &Buffer::Immutable::_getData() const
	{
		if (!_data)
		{
			throw spk::Exception("Buffer has been moved from");
		}
		return *_data;
	}

	Buffer::Mutable::Mutable(std::shared_ptr<Data> data) :
		_data(std::move(data))
	{
	}

	const Buffer::Data &Buffer::Mutable::_getData() const
	{
		if (!_data)
		{
			throw spk::Exception("Buffer has been moved from");
		}
		return *_data;
	}

	Buffer::Data &Buffer::Mutable::_editableData()
	{
		if (!_data)
		{
			throw spk::Exception("Buffer has been moved from");
		}
		return *_data;
	}

	std::byte *Buffer::Mutable::data()
	{
		return _editableData().data();
	}

	void Buffer::Mutable::reserve(std::size_t capacity)
	{
		_editableData().reserve(capacity);
	}

	void Buffer::Mutable::resize(std::size_t size)
	{
		_editableData().resize(size);
	}

	void Buffer::Mutable::clear()
	{
		_editableData().clear();
	}

	void Buffer::Mutable::validate()
	{
		_editableData().validate();
	}

	Buffer::Builder::Builder(Builder &&other) noexcept :
		_data(std::move(other._data)),
		_consumed(std::exchange(other._consumed, true))
	{
	}

	Buffer::Builder &Buffer::Builder::operator=(Builder &&other) noexcept
	{
		if (this != &other)
		{
			_data = std::move(other._data);
			_consumed = std::exchange(other._consumed, true);
		}
		return *this;
	}

	void Buffer::Builder::_check() const
	{
		if (_consumed)
		{
			throw spk::Exception("Buffer builder has been consumed");
		}
	}

	void Buffer::Builder::_ensureData(std::size_t capacity)
	{
		_check();
		if (!_data)
		{
			_data = _pool.obtain(std::max(capacity, std::size_t{1}), [](Data &data) {
				data.reset();
			});
		}
	}

	void Buffer::Builder::_append(std::span<const std::byte> bytes)
	{
		_ensureData(bytes.size());
		_data->append(bytes);
	}

	std::shared_ptr<Buffer::Data> Buffer::Builder::_publish()
	{
		_ensureData(0);
		auto owner = std::make_shared<CapacityPool<Data>::Lease>(std::move(_data));
		auto *data = owner->get();
		_consumed = true;
		return std::shared_ptr<Data>(std::move(owner), data);
	}

	void Buffer::Builder::reserve(std::size_t capacity)
	{
		_ensureData(capacity);
		_data->reserve(capacity);
	}

	void Buffer::Builder::resize(std::size_t size)
	{
		_ensureData(size);
		_data->resize(size);
	}

	void Buffer::Builder::clear()
	{
		_check();
		if (_data)
		{
			_data->clear();
		}
	}

	std::size_t Buffer::Builder::size() const
	{
		_check();
		return _data ? _data->size() : 0;
	}

	bool Buffer::Builder::empty() const
	{
		return size() == 0;
	}

	std::byte *Buffer::Builder::data()
	{
		_check();
		return _data ? _data->data() : nullptr;
	}

	const std::byte *Buffer::Builder::data() const
	{
		_check();
		return _data ? _data->data() : nullptr;
	}

	Buffer::Immutable Buffer::Builder::build() &&
	{
		return Immutable(_publish());
	}

	Buffer::Mutable Buffer::Builder::buildMutable() &&
	{
		return Mutable(_publish());
	}
}
