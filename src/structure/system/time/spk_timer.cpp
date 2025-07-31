#include "structure/system/time/spk_timer.hpp"

#include <algorithm>

namespace spk
{
	Timer::Timer(const Duration &p_expectedDuration) :
		_state(State::Idle),
		_expectedDuration(p_expectedDuration),
		_startTime(),
		_accumulatedTime(0LL, TimeUnit::Nanosecond)
	{
	}

	Timer::Timer() :
		Timer(Duration(0LL, TimeUnit::Nanosecond))
	{
	}

	Timer::State Timer::state() const
	{
		updateTimedOutState();
		return _state;
	}

	Duration Timer::expectedDuration() const
	{
		return _expectedDuration;
	}

	float Timer::elapsedRatio() const
	{
		if (_expectedDuration.nanoseconds == 0LL)
		{
			return 0.0f;
		}

		const long double elapsedNs = static_cast<long double>(elapsed().nanoseconds);
		const long double expectedNs = static_cast<long double>(_expectedDuration.nanoseconds);

		const long double ratio = elapsedNs / expectedNs;

		return static_cast<float>(std::clamp(ratio, 0.0L, 1.0L));
	}

	bool Timer::hasTimedOut() const
	{
		updateTimedOutState();
		return (_state == State::TimedOut);
	}

	void Timer::start()
	{
		if (_state == State::Idle || _state == State::TimedOut)
		{
			_state = State::Running;
			_accumulatedTime = Duration(0LL, TimeUnit::Nanosecond);
			_startTime = SystemUtils::getTime();
		}
	}

	void Timer::stop()
	{
		_state = State::Idle;
		_accumulatedTime = Duration(0LL, TimeUnit::Nanosecond);
		_startTime = {};
	}

	void Timer::pause()
	{
		if (_state == State::Running)
		{
			_accumulatedTime = {static_cast<long double>(_accumulatedTime.nanoseconds + currentRunDuration().nanoseconds), TimeUnit::Nanosecond};
			_state = State::Paused;
		}
	}

	void Timer::resume()
	{
		if (_state == State::Paused)
		{
			_state = State::Running;
			_startTime = SystemUtils::getTime();
		}
	}

	Duration Timer::elapsed() const
	{
		if (_state == State::Running)
		{
			auto diff = currentRunDuration();
			return Duration(_accumulatedTime.nanoseconds + diff.nanoseconds, TimeUnit::Nanosecond);
		}
		else
		{
			return _accumulatedTime;
		}
	}

	Duration Timer::currentRunDuration() const
	{
		if (_state != State::Running)
		{
			return Duration(0LL, TimeUnit::Nanosecond);
		}
		auto now = SystemUtils::getTime();
		auto ns = (now - _startTime).nanoseconds;
		return Duration(ns, TimeUnit::Nanosecond);
	}

	void Timer::updateTimedOutState() const
	{
		if (_state == State::Running)
		{
			auto elapsed = this->elapsed();
			if (elapsed >= _expectedDuration)
			{
				_state = State::TimedOut;
			}
		}
	}

	inline const char *to_string(Timer::State p_state)
	{
		switch (p_state)
		{
		case Timer::State::Idle:
			return "Idle";
		case Timer::State::Running:
			return "Running";
		case Timer::State::Paused:
			return "Paused";
		case Timer::State::TimedOut:
			return "TimedOut";
		default:
			return "UnknownState";
		}
	}

	inline const wchar_t *to_wstring(Timer::State p_state)
	{
		switch (p_state)
		{
		case Timer::State::Idle:
			return L"Idle";
		case Timer::State::Running:
			return L"Running";
		case Timer::State::Paused:
			return L"Paused";
		case Timer::State::TimedOut:
			return L"TimedOut";
		default:
			return L"UnknownState";
		}
	}
}

std::ostream &operator<<(std::ostream &p_os, spk::Timer::State p_state)
{
	return p_os << to_string(p_state);
}

std::wostream &operator<<(std::wostream &p_wos, spk::Timer::State p_state)
{
	return p_wos << to_wstring(p_state);
}