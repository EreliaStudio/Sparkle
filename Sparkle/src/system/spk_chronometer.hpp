#pragma once

#include <chrono>

namespace spk
{
	/**
	 * @class IChronometer<TTimeUnit>
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
	 * auto durationMicro = microChronometer.duration(); // Retrieve the duration in microseconds
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
        IChronometer() = default;
		~IChronometer() = default;

        void start()
        {
            _startTime = std::chrono::steady_clock::now();
			_isRunning = true;
        }

        void stop()
        {
			_isRunning = false;
            _endTime = std::chrono::steady_clock::now();
        }

		bool isRunning()
		{
			return (_isRunning);
		}

        long long duration() const
        {
			if (_isRunning == false)
        	    return (std::chrono::duration_cast<TTimeUnit>(_endTime - _startTime).count());
			else
        	    return (std::chrono::duration_cast<TTimeUnit>(std::chrono::steady_clock::now() - _startTime).count());
        }
	};

	using Chronometer = IChronometer<std::chrono::milliseconds>;
}