#include "system/spk_time_manager.hpp"

#include <thread>

namespace spk
{
	TimeManager::TimeManager():
		_time(0),
		_deltaTime(0)
	{
		_time = _programStartingTime = currentTime();

		update();
	}

	void TimeManager::update()
	{
		long long now = currentTime();

		_deltaTime = now - _time;
		_time = now;
		_programDuration = now - _programStartingTime;
	}

	long long TimeManager::currentTime()
	{
		return (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	}

	void TimeManager::sleepAtLeast(long long p_millisecond)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(p_millisecond));
	}
}