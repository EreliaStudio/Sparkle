#include "network/internal/acknowledgment_deadlines.hpp"

#include "exception.hpp"

namespace spk::NetworkInternal
{
	void AcknowledgmentDeadlines::setTimeout(std::chrono::milliseconds timeout)
	{
		if (timeout.count() <= 0)
		{
			throw Exception("Acknowledgment timeout must be positive.");
		}
		const std::scoped_lock lock(_mutex);
		_timeout = timeout;
	}

	std::chrono::milliseconds AcknowledgmentDeadlines::timeout() const noexcept
	{
		const std::scoped_lock lock(_mutex);
		return _timeout;
	}

	void AcknowledgmentDeadlines::start(std::uint64_t transferID, Clock::time_point now)
	{
		if (transferID == 0)
		{
			throw Exception("Cannot track an invalid transfer ID.");
		}
		const std::scoped_lock lock(_mutex);
		_pending.try_emplace(transferID, Deadline{now, _timeout});
	}

	void AcknowledgmentDeadlines::forget(std::uint64_t transferID) noexcept
	{
		const std::scoped_lock lock(_mutex);
		_pending.erase(transferID);
	}

	std::vector<std::uint64_t> AcknowledgmentDeadlines::expire(Clock::time_point now)
	{
		const std::scoped_lock lock(_mutex);
		std::vector<std::uint64_t> expired;
		for (auto it = _pending.begin(); it != _pending.end();)
		{
			if (now - it->second.started < it->second.duration)
			{
				++it;
				continue;
			}
			expired.push_back(it->first);
			it = _pending.erase(it);
		}
		return expired;
	}

	std::size_t AcknowledgmentDeadlines::pending() const noexcept
	{
		const std::scoped_lock lock(_mutex);
		return _pending.size();
	}

	void AcknowledgmentDeadlines::clear() noexcept
	{
		const std::scoped_lock lock(_mutex);
		_pending.clear();
	}
}
