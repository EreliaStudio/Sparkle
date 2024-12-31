#pragma once

#include "utils/spk_system_utils.hpp"

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
}