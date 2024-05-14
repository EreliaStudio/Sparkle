#include "profiler/spk_trigger_metric.hpp"

namespace spk
{
	void TriggerMetric::trigger(long long p_nbTrigger)
	{
		_currentTriggers += p_nbTrigger;
	}

	void TriggerMetric::save()
	{
		saveValue(_currentTriggers);
		_currentTriggers = 0;
	}

	spk::JSON::Object TriggerMetric::emitReport()
	{		
		spk::JSON::Object result;

		result["Min"].set<long>(static_cast<long>(min()));
		result["Max"].set<long>(static_cast<long>(max()));
		result["Average"].set<float>(average());
		result["Cardinal"].set<long>(static_cast<long>(cardinal()));
		
		return result;
	}
}