#pragma once

#include <chrono>
#include <stdexcept>

namespace spk
{
	class TimeMetrics
	{
	private:
		long long _time;

		long long _deltaTime;

		long long _programStartingTime;

		long long _programDuration;

	public:
		TimeMetrics():
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

		static void sleepAtLeast(long long p_millisecond)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(p_millisecond));
		}
	};
}