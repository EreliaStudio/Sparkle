#pragma once

#include "profiler/spk_imetric.hpp"

namespace spk
{
    /**
	 * @class TriggerMetric
     * @brief A metric class for counting and reporting discrete trigger events, extending IAnalyticalMetric<long long>.
     *
     * TriggerMetric is tailored for scenarios where events or actions need to be counted over time. It provides a simple
     * yet effective way to incrementally track occurrences of specific events, such as user interactions, system alerts,
     * or any discrete actions, and report on them. This class enhances the ability to monitor and analyze the frequency
     * and distribution of these events within an application.
     *
     * It specializes the IAnalyticalMetric template to use long long as the data type, ensuring that it can accommodate
     * a large number of triggers without overflow. The class offers methods to increment the trigger count by a specific
     * amount and to save the current count as a data point for analysis.
     *
     * Usage example within a Profiler context:
     * @code
     * spk::Profiler myProfiler;
     * auto& myTriggerMetric = myProfiler.metric<spk::TriggerMetric>("Event Trigger");
     * 
     * myTriggerMetric.trigger(); // Increment the trigger count by 1
     * myTriggerMetric.trigger(5); // Increment the trigger count by 5
     * 
     * myTriggerMetric.save(); // Save the current state for reporting
     * 
     * spk::JSON::Object report = myProfiler.emitReport(); // Generate and retrieve the aggregated report
     * std::cout << report;
     * @endcode
     *
     * @note This class allows for the real-time tracking of event triggers and integrates seamlessly with a profiling
     * system to provide insights into the event dynamics of an application. The simplicity of the interface makes it
     * suitable for a wide range of applications, from performance monitoring to user interaction tracking.
     *
     * @see IAnalyticalMetric
     */

    class TriggerMetric : public IAnalyticalMetric<long long>
    {
    private:
        long long _currentTriggers = 0;

    public:
        /**
         * @brief Default constructor for TriggerMetric.
         *
         * Initializes a new instance of TriggerMetric with the trigger count set to zero.
         * This metric is ready to track discrete trigger events immediately after instantiation.
         */
        TriggerMetric() = default;

        /**
         * @brief Default destructor for TriggerMetric.
         *
         * Ensures that any resources used by the TriggerMetric instance are properly
         * released. Although this class does not allocate dynamic resources, the
         * destructor is defined for completeness and future extensibility.
         */
        virtual ~TriggerMetric() = default;

        /**
         * @brief Increments the trigger count.
         *
         * This method increments the count of trigger events by a specified amount, allowing
         * for flexible counting of events. By default, it increments the count by one, but
         * can be adjusted to account for multiple triggers at once.
         *
         * @param p_nbTrigger The number of triggers to add to the current count. Defaults to 1.
         */
        void trigger(long long p_nbTrigger = 1);

        /**
         * @brief Saves the current trigger count as a data point.
         *
         * Records the current state of the trigger count, updating the analytical metrics
         * with the new data point. This method is useful for capturing the count at specific
         * moments or intervals, contributing to the statistical analysis of event frequency
         * over time.
         */
        void save();

        /**
         * @brief Generates and returns a report based on the collected trigger data.
         *
         * Constructs a JSON object containing the statistical analysis of the trigger events
         * counted by this metric. The report includes minimum, maximum, and average trigger
         * counts, as well as the total number of data points recorded.
         *
         * @return spk::JSON::Object A JSON object containing the aggregated trigger metrics,
         * providing insights into the event dynamics captured by this metric.
         */
        virtual spk::JSON::Object emitReport() override;
    };
}
