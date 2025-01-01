#include "structure/system/time/spk_timer.hpp"

namespace spk
{
    Timer::Timer(const Duration& p_expectedDuration) :
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
	
	Timer::Timer(long long p_value, TimeUnit p_unit = TimeUnit::Millisecond) :
		Timer(Duration(p_value, p_unit))
	{

	}
	
	Timer::Timer(double p_value, TimeUnit p_unit = TimeUnit::Millisecond) :
		Timer(Duration(p_value, p_unit))
	{

	}

    Timer::State Timer::state()
    {
        updateTimedOutState();
        return _state;
    }

    Duration Timer::expectedDuration() const
    {
        return _expectedDuration;
    }

    bool Timer::hasTimedOut()
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
            _accumulatedTime = {_accumulatedTime.value + currentRunDuration().value, TimeUnit::Nanosecond};
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
            return Duration(_accumulatedTime.value + diff.value, TimeUnit::Nanosecond);
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

    void Timer::updateTimedOutState()
    {
        if (_state == State::Running)
        {
            auto elapsed = this->elapsed();
            if (elapsed.value >= _expectedDuration.value)
            {
                _state = State::TimedOut;
            }
        }
    }


	inline const char* to_string(Timer::State state)
	{
		switch (state)
		{
			case Timer::State::Idle:     return "Idle";
			case Timer::State::Running:  return "Running";
			case Timer::State::Paused:   return "Paused";
			case Timer::State::TimedOut: return "TimedOut";
			default:              return "UnknownState";
		}
	}

	inline const wchar_t* to_wstring(Timer::State state)
	{
		switch (state)
		{
			case Timer::State::Idle:     return L"Idle";
			case Timer::State::Running:  return L"Running";
			case Timer::State::Paused:   return L"Paused";
			case Timer::State::TimedOut: return L"TimedOut";
			default:              return L"UnknownState";
		}
	}
}

std::ostream& operator<<(std::ostream& os, spk::Timer::State state)
{
    return os << to_string(state);
}

std::wostream& operator<<(std::wostream& wos, spk::Timer::State state)
{
    return wos << to_wstring(state);
}