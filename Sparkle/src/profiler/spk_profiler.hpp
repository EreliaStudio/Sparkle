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
		Profiler();
		~Profiler();

		template <typename TMetricType>
		TMetricType& metric(const std::string& p_metricName)
		{
			if (_metrics.contains(p_metricName) == false)
				_metrics[p_metricName] = new TMetricType();
			return (*(static_cast<TMetricType*>(_metrics.at(p_metricName))));
		}

		spk::JSON::Object emitReport();
	};
}