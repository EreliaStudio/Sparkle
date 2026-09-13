#pragma once

#include "core/platform/chronometer.hpp"
#include "exception.hpp"

namespace spk
{
	class Timer
	{
	public:
		using Duration = Chronometer::Duration;

		enum class State
		{
			Off,
			Running,
			TimedOut
		};

	private:
		Chronometer _chrono;
		mutable State _state = State::Off;
		Duration _duration;

	public:
		explicit Timer(Duration duration) :
			_duration(duration)
		{
			if (_duration <= Duration::zero())
			{
				throw spk::Exception("Timer duration must be positive");
			}
		}

		void reset()
		{
			_chrono.reset();
			_state = State::Off;
		}

		void start()
		{
			_chrono.start();
			_state = State::Running;
		}

		[[nodiscard]] Duration stop()
		{
			_state = State::Off;
			return _chrono.stop();
		}

		[[nodiscard]] Duration elapsedTime() const
		{
			return _chrono.elapsedTime();
		}

		[[nodiscard]] State state() const
		{
			if (_state == State::Running && elapsedTime() >= _duration)
			{
				_state = State::TimedOut;
			}

			return _state;
		}

		[[nodiscard]] const Duration &duration() const
		{
			return _duration;
		}
	};
}
