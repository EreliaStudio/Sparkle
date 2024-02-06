#pragma once

#include "profiler/spk_imetric.hpp"

namespace spk
{
    class TriggerMetric : public IAnalyticalMetric<long long>
    {
    private:
        long long _currentTriggers = 0;

    public:
        TriggerMetric() = default;
        virtual ~TriggerMetric() = default;

        void trigger(long long p_nbTrigger = 1);
        void save();

        virtual spk::JSON::Object emitReport() override;
    };
}
