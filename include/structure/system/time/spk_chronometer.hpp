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

		State state() const
		{
			return _state;
		}

	private:
		Duration _currentRunDuration() const;
	};

	inline const char *toString(Chronometer::State p_state);
	inline const wchar_t *toWstring(Chronometer::State p_state);
}

std::ostream &operator<<(std::ostream &p_os, spk::Chronometer::State p_state);
std::wostream &operator<<(std::wostream &p_wos, spk::Chronometer::State p_state);