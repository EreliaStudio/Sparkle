#pragma once

#include "exception.hpp"

#include <chrono>
#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

namespace spk::NetworkInternal
{
	class PageFailureLimiter final
	{
	public:
		using Clock = std::chrono::steady_clock;

		struct Config final
		{
			std::size_t minimumPages = 2;
			std::size_t minimumBytes = 128u * 1024u;
			std::chrono::milliseconds window = std::chrono::seconds(60);
			std::size_t consecutiveFailures = 10;
		};

	private:
		mutable std::mutex _mutex;
		Config _config;
		std::deque<std::pair<Clock::time_point, std::size_t>> _pages;
		std::size_t _bytes = 0;
		std::size_t _consecutive = 0;
		bool _tripped = false;

		void _expire(Clock::time_point now)
		{
			while (!_pages.empty() && now - _pages.front().first >= _config.window)
			{
				_bytes -= _pages.front().second;
				_pages.pop_front();
			}
		}

	public:
		void configure(Config config)
		{
			if (config.minimumPages == 0 || config.minimumBytes == 0 ||
				config.window.count() <= 0 || config.consecutiveFailures == 0)
			{
				throw spk::Exception("Network failure limits must be positive.");
			}
			const std::scoped_lock lock(_mutex);
			_config = config;
			_expire(Clock::now());
		}

		[[nodiscard]] Config config() const
		{
			const std::scoped_lock lock(_mutex);
			return _config;
		}

		[[nodiscard]] bool malformedPage(std::size_t actualBytes, Clock::time_point now = Clock::now())
		{
			const std::scoped_lock lock(_mutex);
			_expire(now);
			_pages.emplace_back(now, actualBytes);
			_bytes += actualBytes;
			_tripped |= _pages.size() >= _config.minimumPages && _bytes >= _config.minimumBytes;
			return _tripped;
		}

		[[nodiscard]] bool failedMessage()
		{
			const std::scoped_lock lock(_mutex);
			++_consecutive;
			_tripped |= _consecutive >= _config.consecutiveFailures;
			return _tripped;
		}

		void successfulMessage()
		{
			const std::scoped_lock lock(_mutex);
			_consecutive = 0;
		}

		[[nodiscard]] bool tripped() const
		{
			const std::scoped_lock lock(_mutex);
			return _tripped;
		}

		void clear()
		{
			const std::scoped_lock lock(_mutex);
			_pages.clear();
			_bytes = 0;
			_consecutive = 0;
			_tripped = false;
		}
	};
}
