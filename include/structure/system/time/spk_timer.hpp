#pragma once

#include "utils/spk_system_utils.hpp"

namespace spk
{

    class Timer
    {
	public:	
		enum class State
		{
			Idle,
			Running,
			Paused,
			TimedOut
		};

    private:
        Duration currentRunDuration() const;
        void updateTimedOutState();

        State _state;
        Duration _expectedDuration;
        Timestamp _startTime;
        Duration _accumulatedTime;

    public:
        explicit Timer(const Duration& p_expectedDuration);
		Timer();
		Timer(long long p_value, TimeUnit p_unit);
		Timer(double p_value, TimeUnit p_unit);

        State state();
        Duration elapsed() const;
        Duration expectedDuration() const;
        bool hasTimedOut();

        void start();
        void stop();
        void pause();
        void resume();

    };

	inline const char* to_string(State state);
	inline const wchar_t* to_wstring(State state);
}

std::ostream& operator<<(std::ostream& os, spk::State state);
std::wostream& operator<<(std::wostream& wos, spk::State state);