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
		using CapacityAccessor = std::function<std::size_t(const TElement &)>;
		using Lease = typename Pool<TElement>::Lease;

	private:
		class State : public std::enable_shared_from_this<State>
		{
		private:
			Factory _factory;
			CapacityAccessor _capacityAccessor;
			std::mutex _mutex;
			std::map<std::size_t, std::unique_ptr<Pool<TElement>>> _pools;

			void _recycle(TElement *element) noexcept
			{
				std::unique_ptr<TElement> owner(element);
				try
				{
					const auto capacity = std::bit_floor(_capacityAccessor(*owner));
					if (capacity != 0)
					{
						auto &destination = pool(capacity);
						destination._state->cache(owner.release());
					}
				} catch (...)
				{
					// Recycling is optional; the element is destroyed on failure.
				}
			}

		public:
			State(Factory factory, CapacityAccessor capacityAccessor) :
				_factory(std::move(factory)),
				_capacityAccessor(std::move(capacityAccessor))
			{
				if (!_factory)
				{
					throw spk::Exception("CapacityPool requires a factory");
				}
			}

			Pool<TElement> &pool(std::size_t capacity)
			{
				const std::scoped_lock lock(_mutex);
				auto it = _pools.find(capacity);
				if (it != _pools.end())
				{
					return *it->second;
				}

				auto result = std::make_unique<Pool<TElement>>([factory = _factory, capacity]() {
					return factory(capacity);
				});
				if (_capacityAccessor)
				{
					result->_state->setRecycler([weak = this->weak_from_this()](TElement *element) noexcept {
						if (auto state = weak.lock())
						{
							state->_recycle(element);
						}
						else
						{
							delete element;
						}
					});
				}
				return *_pools.emplace(capacity, std::move(result)).first->second;
			}
		};

		std::shared_ptr<State> _state;

	public:
		explicit CapacityPool(Factory factory) :
			CapacityPool(std::move(factory), {})
		{
		}

		CapacityPool(Factory factory, CapacityAccessor capacityAccessor) :
			_state(std::make_shared<State>(std::move(factory), std::move(capacityAccessor)))
		{
		}

		CapacityPool(const CapacityPool &) = delete;
		CapacityPool &operator=(const CapacityPool &) = delete;

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
			return _state->pool(capacityFor(count)).obtain(std::forward<TArgs>(args)...);
		}
	};
}
