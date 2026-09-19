#pragma once

#include <chrono>

namespace spk
{
	class Chronometer
	{
	public:
		using Clock = std::chrono::steady_clock;
		using TimePoint = Clock::time_point;
		using Duration = Clock::duration;

	private:
		TimePoint _startingTime{};
		Duration _savedDuration = Duration::zero();
		bool _running = false;

	public:
		Chronometer() = default;

		bool isRunning() const
		{
			return _running;
		}

		void reset()
		{
			_savedDuration = Duration::zero();
			if (_running)
			{
				_startingTime = Clock::now();
			}
		}

		void start()
		{
			if (_running)
			{
				return;
			}

			_startingTime = Clock::now();
			_running = true;
		}

		[[nodiscard]] Duration stop()
		{
			if (_running)
			{
				_savedDuration += currentElapsedTime();
				_running = false;
			}

			return _savedDuration;
		}

		[[nodiscard]] Duration currentElapsedTime() const
		{
			if (!_running)
			{
				return Duration::zero();
			}

			return Clock::now() - _startingTime;
		}

		[[nodiscard]] Duration elapsedTime() const
		{
			return _savedDuration + currentElapsedTime();
		}
	};
}
