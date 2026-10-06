#pragma once

#include <cstdint>
#include <exception.hpp>
#include <limits>

namespace spk::Network
{
	class Sequence final
	{
		std::uint64_t _value;

	public:
		explicit Sequence(std::uint64_t initialValue = 0) :
			_value(initialValue)
		{
		}
		[[nodiscard]] std::uint64_t value() const noexcept
		{
			return _value;
		}
		[[nodiscard]] std::uint64_t next()
		{
			if (_value == std::numeric_limits<std::uint64_t>::max())
			{
				throw spk::Exception("Network sequence exhausted");
			}
			return ++_value;
		}
	};
} // namespace spk::Network
