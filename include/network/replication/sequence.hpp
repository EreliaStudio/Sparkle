#pragma once

#include <cstdint>

namespace spk::Network
{
	class Sequence final
	{
		std::uint64_t _value;

	public:
		explicit Sequence(std::uint64_t initialValue = 0);
		[[nodiscard]] std::uint64_t value() const noexcept;
		[[nodiscard]] std::uint64_t next();
	};
} // namespace spk::Network
