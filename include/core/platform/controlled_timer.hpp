#pragma once

#include "core/platform/timer.hpp"

namespace spk
{
	class ControlledTimer final : public Timer
	{
	private:
		Duration _elapsed = Duration::zero();
		mutable State _state = State::Off;

	public:
		explicit ControlledTimer(Duration duration) :
			Timer(duration)
		{
		}

		void reset() override
		{
			_elapsed = Duration::zero();
			_state = State::Off;
		}

		void start() override
		{
			_state = State::Running;
		}

		[[nodiscard]] Duration stop() override
		{
			_state = State::Off;
			return _elapsed;
		}

		void advance(Duration deltaTime)
		{
			if (deltaTime < Duration::zero())
			{
				throw spk::Exception("ControlledTimer cannot advance backwards");
			}
			if (_state != State::Off)
			{
				_elapsed += deltaTime;
			}
		}

		[[nodiscard]] Duration elapsedTime() const override
		{
			return _elapsed;
		}

		[[nodiscard]] State state() const override
		{
			if (_state == State::Running && _elapsed >= duration())
			{
				_state = State::TimedOut;
			}
			return _state;
		}
	};
}
