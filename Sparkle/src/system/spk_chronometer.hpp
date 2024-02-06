#pragma once

#include <chrono>

namespace spk
{
	template <typename TTimeUnit = std::chrono::milliseconds>
	class IChronometer
	{
	private:
		bool _isRunning = false;
        std::chrono::steady_clock::time_point _startTime;
        std::chrono::steady_clock::time_point _endTime;

    public:
        IChronometer() = default;
		~IChronometer() = default;

        void start()
        {
            _startTime = std::chrono::steady_clock::now();
			_isRunning = true;
        }

        void stop()
        {
			_isRunning = false;
            _endTime = std::chrono::steady_clock::now();
        }

		bool isRunning()
		{
			return (_isRunning);
		}

        long long duration() const
        {
			if (_isRunning == false)
        	    return (std::chrono::duration_cast<TTimeUnit>(_endTime - _startTime).count());
			else
        	    return (std::chrono::duration_cast<TTimeUnit>(std::chrono::steady_clock::now() - _startTime).count());
        }
	};

	using Chronometer = IChronometer<std::chrono::milliseconds>;
}