#include "utils/spk_system_utils.hpp"

#include <cstdint>
#include <windows.h>

#include <stdexcept>

namespace spk
{
	Duration::Duration() :
		value(0)
	{

	}

	Duration::Duration(long long p_value, TimeUnit p_unit)
	{
		switch (p_unit)
		{
			case TimeUnit::Second:
			{
				value = p_value * 1'000'000'000;
				break;
			}
			case TimeUnit::Millisecond:
			{
				value = p_value * 1'000;
				break;
			}
			case TimeUnit::Nanosecond:
			{
				value = p_value;
				break;
			}
			default:
			{
				throw std::runtime_error("Unexpected duration unit value");
			}
		}
	}

	Duration::Duration(double p_value, TimeUnit p_unit)
	{
		switch (p_unit)
		{
			case TimeUnit::Second:
			{
				value = p_value * 1'000'000'000.0;
				break;
			}
			case TimeUnit::Millisecond:
			{
				value = p_value * 1'000.0;
				break;
			}
			case TimeUnit::Nanosecond:
			{
				value = p_value;
				break;
			}
			default:
			{
				throw std::runtime_error("Unexpected duration unit value");
			}
		}
	}

	Timestamp::Timestamp(const Duration& d)
	{
		nanoseconds  = d.value;
		milliseconds = d.value / 1'000'000LL;
		seconds      = static_cast<double>(d.value) / 1'000'000'000.0;
	}

	Timestamp Timestamp::operator+(const Timestamp& other) const
	{
		long long sumNs = this->nanoseconds + other.nanoseconds;
		return fromNanoseconds(sumNs);
	}

	Timestamp Timestamp::operator-(const Timestamp& other) const
	{
		long long diffNs = this->nanoseconds - other.nanoseconds;
		return fromNanoseconds(diffNs);
	}

	Timestamp& Timestamp::operator+=(const Timestamp& other)
	{
		this->nanoseconds += other.nanoseconds;
		this->milliseconds = this->nanoseconds / 1'000'000LL;
		this->seconds      = static_cast<double>(this->nanoseconds) / 1'000'000'000.0;
		return *this;
	}

	Timestamp& Timestamp::operator-=(const Timestamp& other)
	{
		this->nanoseconds -= other.nanoseconds;
		this->milliseconds = this->nanoseconds / 1'000'000LL;
		this->seconds      = static_cast<double>(this->nanoseconds) / 1'000'000'000.0;
		return *this;
	}

	bool Timestamp::operator==(const Timestamp& other) const
	{
		return this->milliseconds == other.milliseconds;
	}

	bool Timestamp::operator!=(const Timestamp& other) const
	{
		return !(*this == other);
	}

	bool Timestamp::operator<(const Timestamp& other) const
	{
		return this->milliseconds < other.milliseconds;
	}

	bool Timestamp::operator>(const Timestamp& other) const
	{
		return this->milliseconds > other.milliseconds;
	}

	bool Timestamp::operator<=(const Timestamp& other) const
	{
		return this->milliseconds <= other.milliseconds;
	}

	bool Timestamp::operator>=(const Timestamp& other) const
	{
		return this->milliseconds >= other.milliseconds;
	}

	Timestamp Timestamp::fromNanoseconds(long long ns)
	{
		Timestamp ts;
		ts.nanoseconds  = ns;
		ts.milliseconds = ns / 1'000'000LL;
		ts.seconds      = static_cast<double>(ns) / 1'000'000'000.0;
		return ts;
	}
}

namespace spk::SystemUtils
{
    spk::Timestamp getTime()
    {
        static bool          initialized       = false;
        static LARGE_INTEGER frequency         = {};
        static double        baseUnixEpochSec  = 0.0;

        if (!initialized)
        {
            ::QueryPerformanceFrequency(&frequency);

            FILETIME fileTime;
            ::GetSystemTimeAsFileTime(&fileTime);

            ULARGE_INTEGER largeInt;
            largeInt.LowPart  = fileTime.dwLowDateTime;
            largeInt.HighPart = fileTime.dwHighDateTime;

            constexpr long long EPOCH_DIFF_100NS = 116444736000000000LL;
            long long currentTimeFileTimeNs =
                (static_cast<long long>(largeInt.QuadPart) - EPOCH_DIFF_100NS) * 100LL;

            double currentTimeFileTimeSec =
                static_cast<double>(currentTimeFileTimeNs) / 1'000'000'000.0;

            LARGE_INTEGER counter;
            ::QueryPerformanceCounter(&counter);

            double qpcTimeNowSec =
                static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);

            baseUnixEpochSec = currentTimeFileTimeSec - qpcTimeNowSec;

            initialized = true;
        }

        LARGE_INTEGER counter;
        ::QueryPerformanceCounter(&counter);

        double qpcTimeSec =
            static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);

        double currentTimeSec = qpcTimeSec + baseUnixEpochSec;

        Timestamp timestamp;
        timestamp.seconds      = currentTimeSec;
        timestamp.milliseconds = static_cast<long long>(currentTimeSec * 1'000.0);
        timestamp.nanoseconds  = static_cast<long long>(currentTimeSec * 1'000'000'000.0);

        return timestamp;
    }
}