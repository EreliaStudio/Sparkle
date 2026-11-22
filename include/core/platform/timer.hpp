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
		Duration _duration;
		mutable State _state = State::Off;

	protected:
		virtual void _startClock()
		{
		}

		[[nodiscard]] virtual Duration _stopClock()
		{
			return elapsedTime();
		}

		virtual void _resetClock() = 0;

	public:
		explicit Timer(Duration duration) :
			_duration(duration)
		{
			if (_duration <= Duration::zero())
			{
				throw spk::Exception("Timer duration must be positive");
			}
		}

		virtual ~Timer() = default;

		void reset()
		{
			_resetClock();
			_state = State::Off;
		}

		void start()
		{
			_startClock();
			_state = State::Running;
		}

		[[nodiscard]] Duration stop()
		{
			_state = State::Off;
			return _stopClock();
		}

		[[nodiscard]] virtual Duration elapsedTime() const = 0;
		[[nodiscard]] bool isElapsed() const
		{
			return elapsedTime() >= duration();
		}

		[[nodiscard]] State state() const
		{
			if (_state == State::Running && isElapsed())
			{
				_state = State::TimedOut;
			}
			return _state;
		}

		[[nodiscard]] const Duration &duration() const noexcept
		{
			return _duration;
		}
	};
}
