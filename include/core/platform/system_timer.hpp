#pragma once

#include "core/platform/chronometer.hpp"
#include "core/platform/timer.hpp"

namespace spk
{
	class SystemTimer final : public Timer
	{
	private:
		Chronometer _chronometer;
		mutable State _state = State::Off;

	public:
		explicit SystemTimer(Duration duration) :
			Timer(duration)
		{
		}

		void reset() override
		{
			_chronometer.reset();
			_state = State::Off;
		}

		void start() override
		{
			_chronometer.start();
			_state = State::Running;
		}

		[[nodiscard]] Duration stop() override
		{
			_state = State::Off;
			return _chronometer.stop();
		}

		[[nodiscard]] Duration elapsedTime() const override
		{
			return _chronometer.elapsedTime();
		}

		[[nodiscard]] State state() const override
		{
			if (_state == State::Running && elapsedTime() >= duration())
			{
				_state = State::TimedOut;
			}
			return _state;
		}
	};
}
