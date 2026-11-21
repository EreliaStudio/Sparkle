#pragma once

#include "core/platform/chronometer.hpp"
#include "core/platform/timer.hpp"

namespace spk
{
	class SystemTimer final : public Timer
	{
	private:
		Chronometer _chronometer;

	protected:
		void _startClock() override
		{
			_chronometer.start();
		}

		[[nodiscard]] Duration _stopClock() override
		{
			return _chronometer.stop();
		}

		void _resetClock() override
		{
			_chronometer.reset();
		}

	public:
		explicit SystemTimer(Duration duration) :
			Timer(duration)
		{
		}

		[[nodiscard]] Duration elapsedTime() const override
		{
			return _chronometer.elapsedTime();
		}
	};
}
