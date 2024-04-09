#pragma once

#include "system/spk_chronometer.hpp"

namespace spk
{
	/**
	 * @class ITimer
	 * 
	 * @tparam TTimeUnit The time unit for measuring intervals, defaulting to milliseconds.
	 * 
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
		/**
		 * @brief Default constructor for ITimer. Initializes the timer without a set duration.
		 */
		ITimer() = default;

		/**
		 * @brief Destructor for ITimer.
		 */
		~ITimer() = default;

		/**
		 * @brief Constructs an ITimer with a specified duration.
		 * 
		 * @param p_duration The duration for the timer, specified in units defined by the TTimeUnit template parameter.
		 *                   The duration is internally converted to the corresponding std::chrono duration type.
		 */
		ITimer(const size_t& p_duration) :
			_duration(std::chrono::duration_cast<TTimeUnit>(TTimeUnit(p_duration)))
		{

		}

		const TTimeUnit& duration() const
		{
			return (_duration);
		}

		/**
		 * @brief Sets the duration of the timer.
		 * 
		 * Allows dynamically updating the timer's duration after instantiation. This method can be called at any time to
		 * reset the duration to a new value, which is useful for reusing the timer with different intervals.
		 * 
		 * @param p_duration The new duration for the timer, specified in units that match the TTimeUnit template parameter.
		 */
		void setDuration(const size_t& p_duration)
		{
			_duration = std::chrono::duration_cast<TTimeUnit>(TTimeUnit(p_duration));
		}

		/**
		 * @brief Checks if the timer's set duration has elapsed.
		 * 
		 * This method compares the elapsed time since the timer was started to the set duration. It provides a way to
		 * determine if a certain period has passed, which can be used to trigger time-dependent actions.
		 * 
		 * @return true if the set duration has elapsed since the timer was started, false otherwise.
		 */
		bool isTimedOut()
		{
			if (this->isRunning() == false)
				return true;
				
			return (this->elapsedTime() >= _duration.count());
		}
	};

	using Timer = ITimer<std::chrono::milliseconds>; //!< A using for a Timer in millisecond
}