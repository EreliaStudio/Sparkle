#pragma once

#include "system/spk_chronometer.hpp"

namespace spk
{
	/**
	 * @class ITimer<TTimeUnit>
	 * @brief A timer class template extending IChronometer for measuring time intervals with a specified duration.
	 *
	 * This class leverages the functionality of IChronometer to provide a simple and efficient way to create timers
	 * that can measure elapsed time against a predefined duration. It is templated on the time unit, allowing for
	 * versatile time duration definitions using the std::chrono library. This class is particularly useful for
	 * implementing timeouts, delays, or measuring whether a certain period has elapsed since the timer was started.
	 *
	 * The timer can be configured with a specific duration at construction or through the setDuration method, and
	 * it offers an isTimedOut method to check if the set duration has passed. This makes ITimer suitable for a wide
	 * range of timing and scheduling tasks within applications, such as game loops, event handling, or any scenario
	 * requiring precise timing control.
	 *
	 * Usage example:
	 * @code
	 * spk::ITimer<std::chrono::seconds> countdownTimer(10); // 10-second timer
	 * countdownTimer.start();
	 * while (!countdownTimer.isTimedOut())
	 * {
	 *     // Perform tasks while waiting for the timer to time out
	 * }
	 * // Timer has timed out
	 * @endcode
	 *
	 * @note The class automatically converts the duration set through the constructor or setDuration method into
	 * the template time unit TTimeUnit, ensuring consistent and accurate timing measurements.
	 *
	 * @see IChronometer
	 */
	template <typename TTimeUnit = std::chrono::milliseconds>
	class ITimer : public IChronometer<TTimeUnit>
	{
	private:
		TTimeUnit _duration;

	public:
		ITimer() = default;
        ~ITimer() = default;

		ITimer(const size_t& p_duration) :
			_duration(std::chrono::duration_cast<TTimeUnit>(TTimeUnit(p_duration)))
		{

		}

		void setDuration(const size_t& p_duration)
		{
			_duration = std::chrono::duration_cast<TTimeUnit>(TTimeUnit(p_duration));
		}

		bool isTimedOut()
		{
			if (this->duration() >= _duration.count())
				return (true);
			return (false);
		}
	};

	using Timer = ITimer<std::chrono::milliseconds>;
}