#pragma once

#include "exception.hpp"

#include <cstddef>
#include <memory>
#include <mutex>

namespace spk::NetworkInternal
{
	class CommandBudget final
	{
	public:
		class Lease final
		{
			friend class CommandBudget;

		private:
			CommandBudget *_budget = nullptr;
			std::size_t _bytes = 0;

		public:
			Lease() = default;
			~Lease()
			{
				if (_budget != nullptr)
				{
					_budget->release(_bytes);
				}
			}
		};

	private:
		mutable std::mutex _mutex;
		std::size_t _maximum;
		std::size_t _used = 0;

	public:
		explicit CommandBudget(std::size_t maximum) :
			_maximum(maximum)
		{
		}

		[[nodiscard]] std::shared_ptr<Lease> reserve(std::size_t bytes)
		{
			auto lease = std::make_shared<Lease>();
			const std::scoped_lock lock(_mutex);
			if (_used > _maximum || bytes > _maximum - _used)
			{
				throw Exception("Network outgoing command queue is saturated.");
			}
			_used += bytes;
			lease->_budget = this;
			lease->_bytes = bytes;
			return lease;
		}

		void release(std::size_t bytes) noexcept
		{
			const std::scoped_lock lock(_mutex);
			_used -= bytes;
		}

		void setMaximum(std::size_t maximum) noexcept
		{
			const std::scoped_lock lock(_mutex);
			_maximum = maximum;
		}

		[[nodiscard]] std::size_t used() const noexcept
		{
			const std::scoped_lock lock(_mutex);
			return _used;
		}
	};
}
