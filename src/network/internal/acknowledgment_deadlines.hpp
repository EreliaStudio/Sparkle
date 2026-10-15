#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace spk::NetworkInternal
{
	class AcknowledgmentDeadlines final
	{
	public:
		using Clock = std::chrono::steady_clock;

	private:
		struct Deadline final
		{
			Clock::time_point started;
			std::chrono::milliseconds duration;
		};

		mutable std::mutex _mutex;
		std::chrono::milliseconds _timeout = std::chrono::seconds(30);
		std::unordered_map<std::uint64_t, Deadline> _pending;

	public:
		void setTimeout(std::chrono::milliseconds timeout);
		[[nodiscard]] std::chrono::milliseconds timeout() const noexcept;
		void start(std::uint64_t transferID, Clock::time_point now = Clock::now());
		void forget(std::uint64_t transferID) noexcept;
		[[nodiscard]] std::vector<std::uint64_t> expire(Clock::time_point now = Clock::now());
		[[nodiscard]] std::size_t pending() const noexcept;
		void clear() noexcept;
	};
}
