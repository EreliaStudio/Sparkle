#include "network/internal/handshake_deadline.hpp"

#include "exception.hpp"

#include <algorithm>

namespace spk::NetworkInternal
{
	HandshakeDeadline::HandshakeDeadline(std::chrono::milliseconds duration, Clock::time_point now) :
		_expiresAt(now + duration)
	{
		if (duration.count() <= 0)
		{
			throw Exception("Handshake timeout must be positive.");
		}
	}

	HandshakeDeadline::Clock::time_point HandshakeDeadline::expiresAt() const noexcept
	{
		return _expiresAt;
	}

	bool HandshakeDeadline::expired(Clock::time_point now) const noexcept
	{
		return now >= _expiresAt;
	}

	std::chrono::milliseconds HandshakeDeadline::remaining(Clock::time_point now) const noexcept
	{
		return std::max(std::chrono::milliseconds::zero(), std::chrono::duration_cast<std::chrono::milliseconds>(_expiresAt - now));
	}}
