#pragma once

#include <chrono>
#include <stdexcept>

namespace spk
{
	/**
	 * @class TimeMetrics
	 * @brief Provides high-resolution timing metrics, including delta time and absolute time measurements.
	 *
	 * This class utilizes the std::chrono high-resolution clock to measure and report time intervals and
	 * timestamps, useful for performance monitoring, animations, physics calculations, and other time-sensitive
	 * applications. It encapsulates the complexity of time measurement, offering a straightforward interface
	 * for accessing delta time (time elapsed between updates) and absolute time since the epoch.
	 *
	 * The class automatically updates its internal timing state on each call to update(), calculating the delta
	 * time since the last update. It provides template methods for accessing these time values in various units
	 * (milliseconds, microseconds, nanoseconds), allowing for flexible and precise time calculations tailored to
	 * specific needs.
	 *
	 * Usage example:
	 * @code
	 * spk::TimeMetrics timeMetrics;
	 * 
	 * // In your update loop:
	 * timeMetrics.update(); // Update the timing information
	 * 
	 * long long deltaTimeMs = timeMetrics.deltaTime<spk::TimeMetrics::Milliseconds>(); // Get delta time in milliseconds
	 * long long absoluteTimeNs = timeMetrics.time<spk::TimeMetrics::Nanoseconds>(); // Get absolute time in nanoseconds
	 * @endcode
	 *
	 * @note This class is designed to be used where precise time measurements are critical, providing high-resolution
	 * time data with minimal overhead. Its versatility makes it suitable for a wide range of applications, from
	 * game development to real-time data processing systems.
	 * @note This class is contained inside the spk::Application, and is automaticaly updated by it. You can access it via spk::Application::activeApplication()->timeMetrics();
	 */
	class TimeMetrics
	{
	private:
		using Clock = std::chrono::high_resolution_clock;
		using TimePoint = std::chrono::time_point<Clock>;
		using Duration = std::chrono::duration<long long>;

		TimePoint _currentTime;
		long long _deltaTime;

	public:
		/**
		 * @brief Type alias for representing time in milliseconds.
		 *
		 * This alias simplifies the use of std::chrono::milliseconds within the TimeMetrics
		 * class, allowing for clear and concise specification of time units when retrieving
		 * delta time and absolute time measurements.
		 */
		using Milliseconds = std::chrono::milliseconds;

		/**
		 * @brief Type alias for representing time in microseconds.
		 *
		 * Similar to Milliseconds, this alias enables the use of std::chrono::microseconds
		 * for time measurements. It provides a finer granularity than milliseconds, useful
		 * for applications requiring high precision in timing calculations.
		 */
		using Microseconds = std::chrono::microseconds;

		/**
		 * @brief Type alias for representing time in nanoseconds.
		 *
		 * This alias represents the finest granularity of time measurement available in
		 * std::chrono, allowing for nanosecond precision. It is particularly useful in
		 * performance-critical sections where the precise measurement of very short
		 * durations is necessary.
		 */
		using Nanoseconds = std::chrono::nanoseconds;


		/**
		 * @brief Initializes the TimeMetrics instance.
		 *
		 * Sets up the initial state of the TimeMetrics object, capturing the current
		 * high-resolution time point and initializing the delta time to zero. This
		 * constructor ensures that the timing measurements start from the point of
		 * object creation.
		 */
		TimeMetrics() :
			_currentTime(Clock::now()),
			_deltaTime(0)
		{
		}

		/**
		 * @brief Updates the timing metrics based on the current time.
		 *
		 * Calculates the delta time since the last update and updates the current time
		 * stamp. This method should be called at the beginning of each frame or update
		 * cycle to ensure accurate timing measurements. The delta time is calculated
		 * in milliseconds and stored for later retrieval.
		 */
		void update()
		{
			TimePoint newTime = Clock::now();
			_deltaTime = std::chrono::duration_cast<Milliseconds>(newTime - _currentTime).count();
			_currentTime = newTime;
		}

		/**
		 * @brief Retrieves the delta time since the last update in the specified time unit.
		 *
		 * Returns the amount of time elapsed since the last call to update(), converted
		 * to the specified time unit (e.g., milliseconds, microseconds, nanoseconds).
		 * This method provides the flexibility to work with time measurements in various
		 * units, depending on the needs of the application.
		 *
		 * @tparam TTimeUnit The desired time unit for the delta time measurement. Defaults to milliseconds.
		 * @return The delta time since the last update, in the specified time unit.
		 */
		template<typename TTimeUnit = Milliseconds>
		long long deltaTime() const
		{
			return std::chrono::duration_cast<TTimeUnit>(Milliseconds(_deltaTime)).count();
		}

		/**
		 * @brief Retrieves the absolute time since the epoch in the specified time unit.
		 *
		 * Returns the current time since the epoch (usually 00:00:00 UTC on 1 January 1970),
		 * converted to the specified time unit (e.g., milliseconds, microseconds, nanoseconds).
		 * This method is useful for obtaining a precise timestamp for logging, timing
		 * operations, or other purposes where absolute time is needed.
		 *
		 * @tparam TTimeUnit The desired time unit for the absolute time measurement. Defaults to milliseconds.
		 * @return The current time since the epoch, in the specified time unit.
		 */
		template<typename TTimeUnit = Milliseconds>
		long long time() const
		{
			return std::chrono::duration_cast<TTimeUnit>(_currentTime.time_since_epoch()).count();
		}

	};
}