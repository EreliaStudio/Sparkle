#include "profiler/spk_profiler.hpp"
#include "application/spk_application.hpp"

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
		result["Total program duration"].set<long>(static_cast<long>(spk::Application::activeApplication()->timeManager().programDuration()));
		return (result);
	}
}