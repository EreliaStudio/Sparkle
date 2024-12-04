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

	void Chronometer::pause()
	{
		if (_state == State::Running)
		{
			_accumulatedTime = Duration(_accumulatedTime.nanoseconds + _currentRunDuration().nanoseconds, TimeUnit::Nanosecond);
			_state = State::Paused;
		}
	}

	void Chronometer::resume()
	{
		if (_state == State::Paused)
		{
			_startTime = SystemUtils::getTime();
			_state = State::Running;
		}
	}

	void Chronometer::restart()
	{
		_accumulatedTime = Duration(0LL, TimeUnit::Nanosecond);
		_startTime = SystemUtils::getTime();
		_state = State::Running;
	}

	Duration Chronometer::elapsedTime() const
	{
		if (_state == State::Running)
		{
			auto currentDiff = _currentRunDuration();
			return (Duration(_accumulatedTime.nanoseconds + currentDiff.nanoseconds, TimeUnit::Nanosecond));
		}
		else
		{
			return (_accumulatedTime);
		}
	}

	Duration Chronometer::_currentRunDuration() const
	{
		if (_state != State::Running)
		{
			return (Duration(0LL, TimeUnit::Nanosecond));
		}

		auto now = SystemUtils::getTime();
		auto diffNs = (now - _startTime).nanoseconds;

		return (Duration(diffNs, TimeUnit::Nanosecond));
	}

	const char *toString(Chronometer::State p_state)
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
	const wchar_t *toWstring(Chronometer::State p_state)
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
	return p_os << toString(p_state);
}
std::wostream &operator<<(std::wostream &p_wos, spk::Chronometer::State p_state)
{
	return p_wos << toWstring(p_state);
}