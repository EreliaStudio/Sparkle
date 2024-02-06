#include "profiler/spk_profiler.hpp"

namespace spk
{
	Profiler::Profiler()
	{

	}
		
	Profiler::~Profiler()
	{

	}
	
	spk::JSON::Object Profiler::emitReport()
	{
		spk::JSON::Object result;

		for (const auto& metricPair : _metrics)
		{
			IMetric* metric = metricPair.second;
			if (metric->shouldEmitReport() == true)
			{
				result[metricPair.first] = metric->emitReport();
			}
		}
		return (result);
	}
}