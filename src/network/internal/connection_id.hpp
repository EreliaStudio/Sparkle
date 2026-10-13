#pragma once

#include "exception.hpp"
#include "network/connection.hpp"

#include <atomic>
#include <limits>

namespace spk::NetworkInternal
{
	inline constexpr Connection::ID ConnectionTokenOffset = 2;
	inline constexpr Connection::ID MaximumConnectionID = std::numeric_limits<Connection::ID>::max() - ConnectionTokenOffset;
	inline std::atomic<Connection::ID> NextConnectionID = 1;

	[[nodiscard]] inline Connection::ID allocateConnectionID(std::atomic<Connection::ID> &nextID)
	{
		auto next = nextID.load(std::memory_order_relaxed);
		while (true)
		{
			if (next == Connection::InvalidID || next > MaximumConnectionID)
			{
				throw Exception("Network connection ID space is exhausted.");
			}
			if (nextID.compare_exchange_weak(
					next, next + 1, std::memory_order_relaxed, std::memory_order_relaxed))
			{
				return next;
			}
		}
	}

	[[nodiscard]] inline Connection::ID allocateConnectionID()
	{
		return allocateConnectionID(NextConnectionID);
	}
}
