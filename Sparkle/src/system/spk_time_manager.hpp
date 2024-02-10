#pragma once

#include <chrono>
#include <stdexcept>
#include "profiler/spk_time_metric.hpp"

namespace spk
{
	/**
     * @class TimeManager
     * @brief Manages time-related functionalities within an application.
     *
     * The TimeManager class provides mechanisms to track the passage of time within an application, including
     * the current time, delta time (the time elapsed between the current frame and the last frame), and the
     * total duration the program has been running. It also provides utility functions such as sleeping for a
     * specified duration.
	 * 
	 * @note This class is contained inside spk::Application, and is accessible via the method const TimeManager& timeManager()
	 * 
	 * @see Application
     */
    class TimeManager
    {
    private:
        long long _time;
        long long _deltaTime;
        long long _programStartingTime;
        long long _programDuration;

    public:
        /**
         * @brief Constructs a TimeManager and initializes timing metrics.
         */
        TimeManager();

        /**
         * @brief Updates the timing metrics.
         *
         * Calculates and updates the current time, delta time, and program duration based on the system's clock.
         */
        void update();

        /**
         * @brief Gets the current system time in milliseconds since the epoch.
         *
         * @return Current system time in milliseconds.
         */
        static long long currentTime();

        /**
         * @brief Gets the current time tracked by the TimeManager.
         *
         * @return The current time in milliseconds since the epoch.
         */
        constexpr const long long& time() const
		{
			return (_time);
		}

        /**
         * @brief Gets the delta time since the last update.
         *
         * @return Delta time in milliseconds.
         */
        constexpr const long long& deltaTime() const
		{
			return (_deltaTime);
		}

        /**
         * @brief Gets the total duration the program has been running.
         *
         * @return Program duration in milliseconds.
         */
        constexpr const long long& programDuration() const
		{
			return (_programDuration);
		}

        /**
         * @brief Pauses the execution for at least the specified duration.
         *
         * @param p_millisecond The minimum amount of time, in milliseconds, to sleep.
         */
        static void sleepAtLeast(long long p_millisecond);
    };
}