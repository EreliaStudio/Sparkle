#pragma once

#include <chrono>
#include <stdexcept>

namespace spk
{
	class TimeMetrics
	{
	private:
		using Clock = std::chrono::high_resolution_clock;
		using TimePoint = std::chrono::time_point<Clock>;
		using Duration = std::chrono::duration<long long>;

		TimePoint _currentTime;
		long long _deltaTime;

	public:
		using Milliseconds = std::chrono::milliseconds;
		using Microseconds = std::chrono::microseconds;
		using Nanoseconds = std::chrono::nanoseconds;

		TimeMetrics() :
			_currentTime(Clock::now()),
			_deltaTime(0)
		{
		}

		void update()
		{
			TimePoint newTime = Clock::now();
			_deltaTime = std::chrono::duration_cast<Milliseconds>(newTime - _currentTime).count();
			_currentTime = newTime;
		}

		template<typename TTimeUnit = Milliseconds>
		long long deltaTime() const
		{
			return std::chrono::duration_cast<TTimeUnit>(Milliseconds(_deltaTime)).count();
		}

		template<typename TTimeUnit = Milliseconds>
		long long time() const
		{
			return std::chrono::duration_cast<TTimeUnit>(_currentTime.time_since_epoch()).count();
		}
	};
}