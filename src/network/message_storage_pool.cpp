#include "network/message.hpp"

#include "exception.hpp"

#include <bit>
#include <limits>
#include <map>
#include <mutex>

namespace
{
	using Storage = spk::Message::Storage;

	[[nodiscard]] std::size_t storagePoolCapacity(std::size_t requestedSize)
	{
		if (requestedSize == 0)
		{
			throw spk::Exception(
				"Network message Storage capacity must be strictly positive");
		}

		constexpr std::size_t highestPowerOfTwo =
			std::size_t{1} << (std::numeric_limits<std::size_t>::digits - 1);

		if (requestedSize > highestPowerOfTwo)
		{
			throw spk::Exception(
				"Network message Storage capacity exceeds the largest representable power-of-two size class");
		}

		return std::bit_ceil(requestedSize);
	}

	class StoragePool : public Storage::Pool
	{
	public:
		explicit StoragePool(std::size_t capacity) :
			Storage::Pool([capacity]() {
				auto *storage = new Storage();
				storage->reserve(capacity);
				return storage;
			})
		{
		}
	};

	class StoragePoolCollection
	{
	private:
		std::map<std::size_t, StoragePool> _collection;
		std::mutex _mutex;

	public:
		[[nodiscard]] Storage::Lease obtain(std::size_t requestedSize)
		{
			const std::size_t capacity =
				storagePoolCapacity(requestedSize);

			StoragePool *pool = nullptr;
			{
				const std::scoped_lock lock(_mutex);
				pool = &_collection
							.try_emplace(capacity, capacity)
							.first->second;
			}

			return pool->obtain(
				[](Storage &storage) {
					storage.clear();
				});
		}
	};

	[[nodiscard]] StoragePoolCollection &storagePools()
	{
		static StoragePoolCollection collection;
		return collection;
	}
}

namespace spk
{
	Message::Storage::Lease Message::_obtainStorage(
		std::size_t minimumCapacity)
	{
		if (minimumCapacity == 0)
		{
			return {};
		}

		return storagePools().obtain(minimumCapacity);
	}
}
