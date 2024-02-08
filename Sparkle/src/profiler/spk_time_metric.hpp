#pragma once

#include "profiler/spk_imetric.hpp"

#include "system/spk_chronometer.hpp"

namespace spk
{
    /**
	 * @class TimeMetric
     * @brief Specialized metric for measuring and reporting time durations, extending IAnalyticalMetric<long long>.
     *
     * This class provides a high-level interface for timing operations within an application, leveraging the Chronometer
     * class for precise time measurements. It is designed to track the duration of specific code segments, tasks, or
     * operations by capturing start and stop times. This class simplifies the process of collecting time-based metrics,
     * offering a straightforward way to start and stop the timer and automatically updating the underlying analytical
     * metrics with the measured durations.
     *
     * Internally, TimeMetric uses a Chronometer to measure time intervals in a high-resolution, platform-independent
     * manner. It extends the IAnalyticalMetric template, specializing it to use long long as the metric type, suitable
     * for representing time durations in the finest resolution offered by the underlying system.
     *
     * TimeMetric instances are typically created and managed by a Profiler, which facilitates the aggregation and
     * reporting of various metrics, including time-based measurements. This integration allows for a centralized
     * approach to monitoring and analyzing performance metrics across an application.
     *
     * Usage example:
     * @code
     * spk::Profiler myProfiler;
     * auto& myTimeMetric = myProfiler.metric<spk::TimeMetric>("Execution Time");
     * myTimeMetric.start(); // Begin timing
     * // Execute code to be measured
     * myTimeMetric.stop(); // End timing
     * spk::JSON::Object report = myProfiler.emitReport(); // Generate and retrieve aggregated report, including timing data
     * std::cout << report;
     * @endcode
     *
     * @note TimeMetric supports thread-local storage for program duration tracking, allowing for accurate and isolated
     * timing measurements across different threads. This feature ensures that timing metrics are consistent and
     * meaningful in multi-threaded applications.
     *
     * @see IAnalyticalMetric<long long> for the base class functionality.
     * @see Chronometer for the precise timing mechanism used.
     * @see Profiler for the management and aggregation of metrics, including TimeMetric.
     */
    class TimeMetric : public IAnalyticalMetric<long long>
    {
	public:
		static inline thread_local long ProgramDuration = 0;
    private:
        Chronometer _chronometer;

    public:
        /**
         * @brief Default constructor for TimeMetric.
         *
         * Initializes a new instance of TimeMetric. Sets up the internal chronometer used
         * to measure time intervals but does not start any timing operation. Use start()
         * to begin timing a code segment or operation.
         */
        TimeMetric() = default;

        /**
         * @brief Default destructor for TimeMetric.
         *
         * Cleans up any resources used by the TimeMetric instance. This is primarily
         * relevant if there are any dynamic allocations or system resources in use,
         * which is not the case here but ensures safe cleanup.
         */
        virtual ~TimeMetric() = default;

        /**
         * @brief Starts the timing operation.
         *
         * Begins measuring a time interval by starting the internal chronometer.
         * This method should be called at the beginning of the code segment or
         * operation you wish to measure. Can be called multiple times for consecutive
         * measurements, with each call resetting the timer.
         */
        void start();

        /**
         * @brief Stops the timing operation.
         *
         * Ends the current timing operation by stopping the internal chronometer and
         * saving the elapsed time since start() was called. The measured duration is
         * automatically added to the analytical metrics for later reporting. This method
         * does not reset the chronometer; start() must be called again to begin a new
         * timing operation.
         */
        void stop();

        /**
         * @brief Generates a report of the time metric.
         *
         * Creates and returns a JSON object containing the aggregated data collected
         * by this TimeMetric instance. This includes minimum, maximum, and average
         * durations, along with the total count of timing operations performed.
         * This method provides a structured format for analyzing and sharing the
         * timing data collected.
         *
         * @return spk::JSON::Object A JSON object containing the timing metrics report,
         * including statistical analysis of the measured durations.
         */
        virtual spk::JSON::Object emitReport() override;

    };
}
