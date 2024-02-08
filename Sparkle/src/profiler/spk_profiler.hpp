#pragma once

#include "profiler/spk_imetric.hpp"
#include <map>

namespace spk
{
	/**
	 * @class Profiler
	 * @brief Manages and aggregates metrics for performance profiling or analytical purposes within an application.
	 *
	 * This class serves as a central manager for metrics that adhere to the IMetric interface. It allows for the
	 * dynamic creation, retrieval, and aggregation of metrics, facilitating comprehensive and customizable reporting.
	 * Metrics are managed on a per-name basis, ensuring unique identification and streamlined access to specific metric
	 * instances. This class simplifies the process of monitoring various performance metrics or other analytical data
	 * across an application, providing a unified interface for data collection and reporting.
	 *
	 * The Profiler supports dynamic metric creation based on template types, allowing for flexible integration of various
	 * metric implementations. It automates the management of metric lifecycles, including instantiation and retrieval,
	 * while providing a mechanism for emitting aggregated reports of collected data in a structured JSON format.
	 *
	 * Usage example:
	 * @code
	 * spk::Profiler myProfiler;
	 * auto& cpuMetric = myProfiler.metric<CPUMetric>("CPU Usage");
	 * 
	 * cpuMetric.record(usageData);
	 * 
	 * spk::JSON::Object report = myProfiler.emitReport();
	 * 
	 * std::cout << report << std::endl;
	 * @endcode
	 *
	 * @note The Profiler class assumes ownership of metric instances and is responsible for their lifecycle management,
	 * including proper cleanup to prevent memory leaks. It is designed for ease of use and extension, supporting diverse
	 * metric types through a generic interface.
	 *
	 * @see IMetric
	 */
	class Profiler
	{
	private:
		std::map<std::string, IMetric*> _metrics;

	public:
		/**
		 * @brief Constructs a Profiler instance.
		 *
		 * Initializes the profiler with an empty set of metrics. This constructor prepares
		 * the profiler for metric management, enabling the dynamic addition and retrieval
		 * of metrics based on unique names.
		 */
		Profiler();

		/**
		 * @brief Destructs the Profiler instance.
		 *
		 * Cleans up all managed metric instances by deallocating memory and clearing the
		 * internal map of metrics. This ensures that no memory leaks occur from dynamically
		 * allocated metric objects.
		 */
		~Profiler();

		/**
		 * @brief Retrieves or creates a metric of a specific type associated with a given name.
		 *
		 * This method allows for the dynamic management of metrics. If a metric with the specified
		 * name exists, it is retrieved; otherwise, a new metric of type TMetricType is created,
		 * added to the profiler's management, and returned. This enables flexible and on-demand
		 * metric tracking within the application.
		 *
		 * @tparam TMetricType The type of the metric to retrieve or create. Must implement the IMetric interface.
		 * @param p_metricName The unique name associated with the metric.
		 * @return TMetricType& A reference to the metric instance of the requested type and name.
		 */
		template <typename TMetricType>
		TMetricType& metric(const std::string& p_metricName)
		{
			if (_metrics.contains(p_metricName) == false)
				_metrics[p_metricName] = new TMetricType();
			return (*(static_cast<TMetricType*>(_metrics.at(p_metricName))));
		}

		/**
		 * @brief Generates an aggregated report of all managed metrics.
		 *
		 * Iterates over all managed metrics, invoking their emitReport() method to generate
		 * individual reports, and then aggregates these reports into a single JSON object. This
		 * method provides a unified view of the collected data across all metrics, facilitating
		 * comprehensive analysis and reporting.
		 *
		 * @return spk::JSON::Object A JSON object containing the aggregated report from all metrics.
		 */
		spk::JSON::Object emitReport();
	};
}