#pragma once

#include "profiler/spk_imetric.hpp"

#include "system/spk_chronometer.hpp"

namespace spk
{
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
