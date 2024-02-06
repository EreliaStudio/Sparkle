#pragma once

#include "system/spk_chronometer.hpp"

namespace spk
{
	template <typename TTimeUnit = std::chrono::milliseconds>
	class ITimer : public IChronometer<TTimeUnit>
	{
	private:
		TTimeUnit _duration;

	public:
		ITimer() = default;
        ~ITimer() = default;

		ITimer(const size_t& p_duration) :
			_duration(std::chrono::duration_cast<TTimeUnit>(TTimeUnit(p_duration)))
		{

		}

		void setDuration(const size_t& p_duration)
		{
			_duration = std::chrono::duration_cast<TTimeUnit>(TTimeUnit(p_duration));
		}

		bool isTimedOut()
		{
			if (this->duration() >= _duration.count())
				return (true);
			return (false);
		}
	};

	using Timer = ITimer<std::chrono::milliseconds>;
}