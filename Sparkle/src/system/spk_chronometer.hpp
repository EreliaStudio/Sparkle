#pragma once

#include <chrono>

namespace spk
{
	/**
	 * @class IChronometer
	 * 
	 * @tparam TTimeUnit The time unit for measuring intervals, defaulting to milliseconds.
	 * 
	 * @brief A generic chronometer template class for measuring time intervals with precision.
	 *
	 * This class is designed as a template class that allows for precise measurement of elapsed time intervals,
	 * customizable with different time units (e.g., milliseconds, microseconds, nanoseconds) through the std::chrono
	 * library. It provides a simple interface to start and stop the timer, check if the timer is running, and retrieve
	 * the duration of the measured interval in the specified time unit.
	 *
	 * This class is ideal for performance measurements, timing operations, or any scenario where precise timing is
	 * essential. It uses std::chrono::steady_clock to ensure that the time measurements are monotonic and not subject
	 * to system time adjustments or clock skew.
	 *
	 * Usage example:
	 * @code
	 * spk::IChronometer<std::chrono::microseconds> microChronometer;
	 * microChronometer.start(); // Start timing
	 * // Code block to measure
	 * microChronometer.stop(); // Stop timing
	 * auto durationMicro = microChronometer.elapsedTime(); // Retrieve the duration in microseconds
	 * std::cout << "Duration: " << durationMicro << " microseconds" << std::endl;
	 * @endcode
	 *
	 * @note The class template parameter TTimeUnit determines the unit of time for the duration() method's return value,
	 * allowing for flexible and precise timing measurements according to the application's requirements.
	 */
	template <typename TTimeUnit = std::chrono::milliseconds>
	class IChronometer
	{
	private:
		bool _isRunning = false;
		std::chrono::steady_clock::time_point _startTime;
		std::chrono::steady_clock::time_point _endTime;

	public:
		/**
		 * @brief Default constructor for IChronometer.
		 */
		IChronometer() = default;

		/**
		 * @brief Destructor for IChronometer.
		 */
		~IChronometer() = default;

		/**
		 * @brief Starts the chronometer.
		 *
		 * Marks the start time using std::chrono::steady_clock. This function sets the chronometer
		 * to running state, allowing duration measurements to be made from this point.
		 */
		void start()
		{
			_startTime = std::chrono::steady_clock::now();
			_isRunning = true;
		}

		/**
		 * @brief Stops the chronometer.
		 *
		 * Marks the end time using std::chrono::steady_clock and sets the chronometer to not running state.
		 * Duration measurements will calculate the time interval between the start and this stop time.
		 */
		void stop()
		{
			_endTime = std::chrono::steady_clock::now();
			_isRunning = false;
		}

		/**
		 * @brief Checks if the chronometer is currently running.
		 *
		 * @return true if the chronometer is running, false otherwise.
		 */
		bool isRunning()
		{
			return _isRunning;
		}

		/**
		 * @brief Calculates the duration between the start and stop times in the specified time unit.
		 *
		 * If the chronometer is still running, it calculates the duration from the start time to the current time.
		 * If the chronometer has been stopped, it calculates the duration between the start and stop times.
		 *
		 * @return The duration between the start and stop times (or current time if still running), converted to
		 *		 the specified TTimeUnit.
		 */
		long long elapsedTime() const
		{
			if (!_isRunning)
				return std::chrono::duration_cast<TTimeUnit>(_endTime - _startTime).count();
			else
				return std::chrono::duration_cast<TTimeUnit>(std::chrono::steady_clock::now() - _startTime).count();
		}
	};

	// Aliases for convenience and readability, specifying the time unit for the Chronometer.
	using Chronometer = IChronometer<std::chrono::milliseconds>;
}