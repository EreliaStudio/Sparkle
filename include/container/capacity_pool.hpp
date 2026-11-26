#pragma once

#include <bit>
#include <cstddef>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

#include "container/pool.hpp"

namespace spk
{
	template <typename TElement>
	class CapacityPool
	{
	public:
		using Factory = std::function<TElement *(std::size_t)>;
		using Lease = typename Pool<TElement>::Lease;

	private:
		Factory _factory;
		std::mutex _mutex;
		std::map<std::size_t, std::unique_ptr<Pool<TElement>>> _pools;

		Pool<TElement> &_pool(std::size_t capacity)
		{
			const std::scoped_lock lock(_mutex);
			auto it = _pools.find(capacity);
			if (it == _pools.end())
			{
				it = _pools.emplace(capacity, std::make_unique<Pool<TElement>>([factory = _factory, capacity]() {
										return factory(capacity);
									}))
						 .first;
			}
			return *it->second;
		}

	public:
		explicit CapacityPool(Factory factory) :
			_factory(std::move(factory))
		{
			if (!_factory)
			{
				throw spk::Exception("CapacityPool requires a factory");
			}
		}

		[[nodiscard]] static std::size_t capacityFor(std::size_t count)
		{
			if (count == 0 || count > (std::size_t{1} << (std::numeric_limits<std::size_t>::digits - 1)))
			{
				throw spk::Exception("Invalid CapacityPool element count");
			}
			return std::bit_ceil(count);
		}

		template <typename... TArgs>
		[[nodiscard]] Lease obtain(std::size_t count, TArgs &&...args)
		{
			return _pool(capacityFor(count)).obtain(std::forward<TArgs>(args)...);
		}
	};
}
