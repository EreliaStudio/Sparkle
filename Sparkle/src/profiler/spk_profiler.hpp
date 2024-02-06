#pragma once

#include "profiler/spk_imetric.hpp"
#include <map>

namespace spk
{
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