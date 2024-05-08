#include "profiler/spk_time_metric.hpp"
#include "application/spk_application.hpp"

namespace spk
{
	void TimeMetric::start()
	{
		_chronometer.start();
	}
	
	void TimeMetric::stop()
	{
		_chronometer.stop();
		saveValue(_chronometer.elapsedTime());
	}

	spk::JSON::Object TimeMetric::emitReport()
	{
		spk::JSON::Object result;

		result["Min"].set<long>(static_cast<long>(min()));
		result["Max"].set<long>(static_cast<long>(max()));
		result["Average"].set<float>(average());
		result["CPU usage"].set<float>(average() * 100 / (spk::Application::activeApplication()->timeManager().programDuration() * 1000000));
		result["Cardinal"].set<long>(static_cast<long>(cardinal()));
		
		return result;
	}
}