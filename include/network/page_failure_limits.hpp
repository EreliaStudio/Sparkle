#pragma once

#include <chrono>
#include <cstddef>

namespace spk
{
	struct PageFailureLimits final
	{
		std::size_t minimumPages = 2;
		std::size_t minimumBytes = 128u * 1024u;
		std::chrono::milliseconds window = std::chrono::seconds(60);
		std::size_t consecutiveFailures = 10;

		[[nodiscard]] bool valid() const noexcept
		{
			return minimumPages != 0 && minimumBytes != 0 &&
				window.count() > 0 && consecutiveFailures != 0;
		}
	};
}
