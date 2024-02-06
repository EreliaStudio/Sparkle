#pragma once

#include <chrono>
#include <stdexcept>

namespace spk
{
	/**
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
		using Milliseconds = std::chrono::milliseconds;
		using Microseconds = std::chrono::microseconds;
		using Nanoseconds = std::chrono::nanoseconds;

		TimeMetrics() :
			_currentTime(Clock::now()),
			_deltaTime(0)
		{
		}

		void update()
		{
			TimePoint newTime = Clock::now();
			_deltaTime = std::chrono::duration_cast<Milliseconds>(newTime - _currentTime).count();
			_currentTime = newTime;
		}

		template<typename TTimeUnit = Milliseconds>
		long long deltaTime() const
		{
			return std::chrono::duration_cast<TTimeUnit>(Milliseconds(_deltaTime)).count();
		}

		template<typename TTimeUnit = Milliseconds>
		long long time() const
		{
			return std::chrono::duration_cast<TTimeUnit>(_currentTime.time_since_epoch()).count();
		}
	};
}