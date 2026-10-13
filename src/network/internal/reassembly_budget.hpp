#pragma once

#include "exception.hpp"

#include <cstddef>
#include <mutex>

namespace spk::NetworkInternal
{
	class ReassemblyBudget final
	{
	private:
		mutable std::mutex _mutex;
		std::size_t _maximum;
		std::size_t _used = 0;

	public:
		explicit ReassemblyBudget(std::size_t maximum = 2ull * 1024ull * 1024ull * 1024ull) :
			_maximum(maximum)
		{
			if (maximum == 0)
			{
				throw Exception("Reassembly memory budget must be positive.");
			}
		}

		[[nodiscard]] bool reserve(std::size_t bytes)
		{
			const std::scoped_lock lock(_mutex);
			if (_used > _maximum || bytes > _maximum - _used)
			{
				return false;
			}
			_used += bytes;
			return true;
		}

		void release(std::size_t bytes) noexcept
		{
			const std::scoped_lock lock(_mutex);
			_used -= bytes;
		}

		void setMaximum(std::size_t bytes)
		{
			if (bytes == 0)
			{
				throw Exception("Reassembly memory budget must be positive.");
			}
			const std::scoped_lock lock(_mutex);
			_maximum = bytes;
		}

		[[nodiscard]] std::size_t used() const noexcept
		{
			const std::scoped_lock lock(_mutex);
			return _used;
		}
	};
}
