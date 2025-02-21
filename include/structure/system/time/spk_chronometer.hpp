#pragma once

#include "utils/spk_system_utils.hpp"

#include <iostream>

namespace spk
{
    

    class Chronometer
    {
    public:
		enum class State
		{
			Idle,
			Running,
			Paused
		};

	private:
		State _state;
		Timestamp _startTime;
		Duration _accumulatedTime;

    public:
        Chronometer();

        void start();
        void stop();
        void pause();
        void resume();
        void restart();

        Duration elapsedTime() const;

        State state() const { return _state; }

    private:
        Duration currentRunDuration() const;
    };

	inline const char* to_string(Chronometer::State state);
	inline const wchar_t* to_wstring(Chronometer::State state);
}

std::ostream& operator<<(std::ostream& os, spk::Chronometer::State state);
std::wostream& operator<<(std::wostream& wos, spk::Chronometer::State state);