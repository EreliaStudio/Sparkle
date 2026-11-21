#pragma once

#include "core/platform/timer.hpp"

namespace spk
{
	class ControlledTimer final : public Timer
	{
	private:
		Duration _elapsed = Duration::zero();

	protected:
		void _resetClock() override
		{
			_elapsed = Duration::zero();
		}

	public:
		explicit ControlledTimer(Duration duration) :
			Timer(duration)
		{
		}

		void advance(Duration deltaTime)
		{
			if (deltaTime < Duration::zero())
			{
				throw spk::Exception("ControlledTimer cannot advance backwards");
			}
			if (state() != State::Off)
			{
				_elapsed += deltaTime;
			}
		}

		[[nodiscard]] Duration elapsedTime() const override
		{
			return _elapsed;
		}	};
}
