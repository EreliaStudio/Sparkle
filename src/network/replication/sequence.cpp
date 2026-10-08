#include "network/replication/sequence.hpp"

#include "exception.hpp"
#include <limits>

namespace spk::Network
{
	Sequence::Sequence(std::uint64_t initialValue) :
		_value(initialValue)
	{
	}

	std::uint64_t Sequence::value() const noexcept
	{
		return _value;
	}

	std::uint64_t Sequence::next()
	{
		if (_value == std::numeric_limits<std::uint64_t>::max())
		{
			throw spk::Exception("Network sequence exhausted");
		}
		return ++_value;
	}
}
