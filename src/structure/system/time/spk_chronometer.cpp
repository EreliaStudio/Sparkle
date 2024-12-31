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
            _accumulatedTime = Duration(_accumulatedTime.value + currentRunDuration().value, TimeUnit::Nanosecond);
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
            return (Duration(_accumulatedTime.value + currentDiff.value, TimeUnit::Nanosecond));
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

        auto now    = SystemUtils::getTime();
        auto diffNs = (now - _startTime).nanoseconds;
		
		return (Duration(diffNs, TimeUnit::Nanosecond));
    }
}