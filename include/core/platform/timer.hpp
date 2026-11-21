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

		virtual void reset() = 0;
		virtual void start() = 0;
		[[nodiscard]] virtual Duration stop() = 0;
		[[nodiscard]] virtual Duration elapsedTime() const = 0;
		[[nodiscard]] virtual State state() const = 0;

		[[nodiscard]] const Duration &duration() const noexcept
		{
			return _duration;
		}
	};
}
