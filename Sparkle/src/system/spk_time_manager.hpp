#pragma once

#include <chrono>
#include <stdexcept>
#include "profiler/spk_time_metric.hpp"

namespace spk
{
	class TimeManager
	{
	private:
		long long _time;

		long long _deltaTime;

		long long _programStartingTime;

		long long _programDuration;

	public:
		TimeManager():
			_time(0),
			_deltaTime(0)
		{
			_time = _programStartingTime = currentTime();

			update();
		}

		void update()
		{
			long long now = currentTime();

			_deltaTime = now - _time;
			_time = now;
			_programDuration = now - _programStartingTime;
		}

		static long long currentTime()
		{
			return (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
		}

		constexpr const long long& time() const { return (_time); }

		constexpr const long long& deltaTime() const { return (_deltaTime); }

		constexpr const long long& programDuration() const { return (_programDuration); }

		static void sleepAtLeast(long long p_millisecond)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(p_millisecond));
		}
	};
}