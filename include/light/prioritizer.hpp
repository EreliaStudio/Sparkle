#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "light/selection.hpp"

namespace spk::Light
{
	// Stateful policy is owned by Channel; this operation only ranks captured values.
	class Prioritizer
	{
	public:
		[[nodiscard]] static std::vector<Selection::Entry> select(
			std::vector<Selection::Entry> candidates,
			const std::vector<std::uint64_t> &previouslyRetained,
			std::size_t limit,
			float hysteresis,
			bool resetHistory);
	};
}
