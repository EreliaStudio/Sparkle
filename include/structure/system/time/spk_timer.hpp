#pragma once

#include "utils/spk_system_utils.hpp"

#include <iostream>

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
		Duration _currentRunDuration() const;
		void _updateTimedOutState() const;

		mutable State _state;
		Duration _expectedDuration;
		Timestamp _startTime;
		Duration _accumulatedTime;

	public:
		explicit Timer(const Duration &p_expectedDuration);
		Timer();

		State state() const;
		Duration elapsed() const;
		Duration expectedDuration() const;
		float elapsedRatio() const;
		bool hasTimedOut() const;

		void start();
		void stop();
		void pause();
		void resume();
	};

	inline const char *toString(Timer::State p_state);
	inline const wchar_t *toWstring(Timer::State p_state);
}

std::ostream &operator<<(std::ostream &p_os, spk::Timer::State p_state);
std::wostream &operator<<(std::wostream &p_wos, spk::Timer::State p_state);