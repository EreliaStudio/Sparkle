#pragma once

#include "exception.hpp"
#include "network/connection.hpp"

#include <atomic>
#include <limits>

namespace spk::NetworkInternal
{
	inline std::atomic<Connection::ID> NextConnectionID = 1;

	[[nodiscard]] inline Connection::ID allocateConnectionID()
	{
		auto next = NextConnectionID.load(std::memory_order_relaxed);
		while (true)
		{
			if (next >= std::numeric_limits<Connection::ID>::max())
			{
				throw Exception("Network connection ID space is exhausted.");
			}
			if (NextConnectionID.compare_exchange_weak(
					next, next + 1,
					std::memory_order_relaxed,
					std::memory_order_relaxed))
			{
				return next;
			}
		}
	}
}
