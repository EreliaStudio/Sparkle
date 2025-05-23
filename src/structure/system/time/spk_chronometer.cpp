#include "structure/system/time/spk_chronometer.hpp"

namespace spk
{
	Chronometer::Chronometer() :
		_state(State::Idle),
		_startTime(),
		_accumulatedTime(0LL, TimeUnit::Nanosecond)
	{
	}

	void Chronometer::start()
	{
		if (_state == State::Idle)
		{
			_state = State::Running;
			_startTime = SystemUtils::getTime();
		}
	}

	void Chronometer::stop()
	{
		_state = State::Idle;
		_accumulatedTime = Duration(0LL, TimeUnit::Nanosecond);
		_startTime = {};
	}

	inline void Chronometer::pause()
	{
		if (_state == State::Running)
		{
			_accumulatedTime = Duration(_accumulatedTime.nanoseconds + currentRunDuration().nanoseconds, TimeUnit::Nanosecond);
			_state = State::Paused;
		}
	}

	inline void Chronometer::resume()
	{
		if (_state == State::Paused)
		{
			_startTime = SystemUtils::getTime();
			_state = State::Running;
		}
	}

	inline void Chronometer::restart()
	{
		_accumulatedTime = Duration(0LL, TimeUnit::Nanosecond);
		_startTime = SystemUtils::getTime();
		_state = State::Running;
	}

	inline Duration Chronometer::elapsedTime() const
	{
		if (_state == State::Running)
		{
			auto currentDiff = currentRunDuration();
			return (Duration(_accumulatedTime.nanoseconds + currentDiff.nanoseconds, TimeUnit::Nanosecond));
		}
		else
		{
			return (_accumulatedTime);
		}
	}

	inline Duration Chronometer::currentRunDuration() const
	{
		if (_state != State::Running)
		{
			return (Duration(0LL, TimeUnit::Nanosecond));
		}

		auto now = SystemUtils::getTime();
		auto diffNs = (now - _startTime).nanoseconds;

		return (Duration(diffNs, TimeUnit::Nanosecond));
	}

	const char *to_string(Chronometer::State p_state)
	{
		switch (p_state)
		{
		case Chronometer::State::Idle:
			return "Idle";
		case Chronometer::State::Running:
			return "Running";
		case Chronometer::State::Paused:
			return "Paused";
		default:
			return "UnknownState";
		}
	}
	const wchar_t *to_wstring(Chronometer::State p_state)
	{
		switch (p_state)
		{
		case Chronometer::State::Idle:
			return L"Idle";
		case Chronometer::State::Running:
			return L"Running";
		case Chronometer::State::Paused:
			return L"Paused";
		default:
			return L"UnknownState";
		}
	}
}

std::ostream &operator<<(std::ostream &p_os, spk::Chronometer::State p_state)
{
	return p_os << to_string(p_state);
}
std::wostream &operator<<(std::wostream &p_wos, spk::Chronometer::State p_state)
{
	return p_wos << to_wstring(p_state);
}