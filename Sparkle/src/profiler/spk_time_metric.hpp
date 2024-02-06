#pragma once

#include "profiler/spk_imetric.hpp"

#include "system/spk_chronometer.hpp"

namespace spk
{
    /**
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
        TimeMetric() = default;
        virtual ~TimeMetric() = default;

		void start();
		void stop();

        virtual spk::JSON::Object emitReport() override;
    };
}
