#include "utils/spk_system_utils.hpp"

#include <cstdint>
#include <windows.h>

#include <stdexcept>

#include "structure/system/spk_exception.hpp"

namespace spk
{
	Duration::Duration()
	{
		seconds = 0;
		milliseconds = 0;
		nanoseconds = 0;
	}

	Duration::Duration(long double p_value, TimeUnit p_unit)
	{
		switch (p_unit)
		{
		case TimeUnit::Second:
			seconds = p_value;
			milliseconds = static_cast<long long>(p_value * 1'000);
			nanoseconds = static_cast<long long>(p_value * 1'000'000'000);
			break;
		case TimeUnit::Millisecond:
			seconds = p_value / 1'000.0;
			milliseconds = static_cast<long long>(p_value);
			nanoseconds = static_cast<long long>(p_value * 1'000'000);
			break;
		case TimeUnit::Nanosecond:
			seconds = p_value / 1'000'000'000.0;
			milliseconds = static_cast<long long>(p_value / 1'000'000);
			nanoseconds = static_cast<long long>(p_value);
			break;
		default:
			GENERATE_ERROR("Unexpected duration unit value");
		}
	}

       Duration Duration::operator-() const
       {
               return (Duration(-nanoseconds, TimeUnit::Nanosecond));
       }

	Duration Duration::operator+(const Duration &p_other) const
	{
		return (Duration(nanoseconds + p_other.nanoseconds, TimeUnit::Nanosecond));
	}

	Duration Duration::operator-(const Duration &p_other) const
	{
		return (Duration(nanoseconds - p_other.nanoseconds, TimeUnit::Nanosecond));
	}

	Duration &Duration::operator+=(const Duration &p_other)
	{
		seconds += p_other.seconds;
		milliseconds += p_other.milliseconds;
		nanoseconds += p_other.nanoseconds;
		return (*this);
	}
	Duration &Duration::operator-=(const Duration &p_other)
	{
		seconds -= p_other.seconds;
		milliseconds -= p_other.milliseconds;
		nanoseconds -= p_other.nanoseconds;
		return (*this);
	}

	bool Duration::operator==(const Duration &p_other) const
	{
		return (nanoseconds == p_other.nanoseconds);
	}
	bool Duration::operator!=(const Duration &p_other) const
	{
		return (nanoseconds != p_other.nanoseconds);
	}

	bool Duration::operator<(const Duration &p_other) const
	{
		return (nanoseconds < p_other.nanoseconds);
	}
	bool Duration::operator>(const Duration &p_other) const
	{
		return (nanoseconds > p_other.nanoseconds);
	}
	bool Duration::operator<=(const Duration &p_other) const
	{
		return (nanoseconds <= p_other.nanoseconds);
	}
	bool Duration::operator>=(const Duration &p_other) const
	{
		return (nanoseconds >= p_other.nanoseconds);
	}

	Timestamp::Timestamp(const Duration &p_duration)
	{
		nanoseconds = p_duration.nanoseconds;
		milliseconds = p_duration.milliseconds;
		seconds = seconds;
	}

	Duration Timestamp::operator-(const Timestamp &p_other) const
	{
		return (Duration(nanoseconds - p_other.nanoseconds, TimeUnit::Nanosecond));
	}

	Timestamp Timestamp::operator+(const Duration &p_other) const
	{
		long long sumNs = this->nanoseconds + p_other.nanoseconds;
return (fromNanoseconds(sumNs));
	}

	Timestamp Timestamp::operator-(const Duration &p_other) const
	{
		long long diffNs = this->nanoseconds - p_other.nanoseconds;
return (fromNanoseconds(diffNs));
	}

	Timestamp &Timestamp::operator+=(const Duration &p_other)
	{
		this->nanoseconds += p_other.nanoseconds;
		this->milliseconds += p_other.milliseconds;
		this->seconds += p_other.seconds;
return (*this);
	}

	Timestamp &Timestamp::operator-=(const Duration &p_other)
	{
		this->nanoseconds -= p_other.nanoseconds;
		this->milliseconds -= p_other.milliseconds;
		this->seconds -= p_other.seconds;
return (*this);
	}

	bool Timestamp::operator==(const Timestamp &p_other) const
	{
return (this->milliseconds == p_other.milliseconds);
	}

	bool Timestamp::operator!=(const Timestamp &p_other) const
	{
		return !(*this == p_other);
	}

	bool Timestamp::operator<(const Timestamp &p_other) const
	{
return (this->milliseconds < p_other.milliseconds);
	}

	bool Timestamp::operator>(const Timestamp &p_other) const
	{
return (this->milliseconds > p_other.milliseconds);
	}

	bool Timestamp::operator<=(const Timestamp &p_other) const
	{
return (this->milliseconds <= p_other.milliseconds);
	}

	bool Timestamp::operator>=(const Timestamp &p_other) const
	{
return (this->milliseconds >= p_other.milliseconds);
	}

       Timestamp Timestamp::fromNanoseconds(long long p_nanoseconds)
       {
               Timestamp timestampResult;
               timestampResult.nanoseconds = p_nanoseconds;
               timestampResult.milliseconds = p_nanoseconds / 1'000'000LL;
               timestampResult.seconds = static_cast<double>(p_nanoseconds) / 1'000'000'000.0;
               return (timestampResult);
       }
}

spk::Duration operator""_s(long double p_value)
{
       return (spk::Duration(static_cast<long double>(p_value), spk::TimeUnit::Second));
}

spk::Duration operator""_ms(unsigned long long p_value)
{
       return (spk::Duration(static_cast<long double>(p_value), spk::TimeUnit::Millisecond));
}

spk::Duration operator""_ns(unsigned long long p_value)
{
       return (spk::Duration(static_cast<long double>(p_value), spk::TimeUnit::Nanosecond));
}

namespace spk::SystemUtils
{
       spk::Timestamp getTime()
	{
		static bool initialized = false;
		static LARGE_INTEGER frequency = {};
		static double baseUnixEpochSec = 0.0;

		if (!initialized)
		{
			::QueryPerformanceFrequency(&frequency);

			FILETIME fileTime;
			::GetSystemTimeAsFileTime(&fileTime);

			ULARGE_INTEGER largeInt;
			largeInt.LowPart = fileTime.dwLowDateTime;
			largeInt.HighPart = fileTime.dwHighDateTime;

			constexpr long long epochDiff100Ns = 116444736000000000LL;
			long long currentTimeFileTimeNs = (static_cast<long long>(largeInt.QuadPart) - epochDiff100Ns) * 100LL;

			double currentTimeFileTimeSec = static_cast<double>(currentTimeFileTimeNs) / 1'000'000'000.0;

			LARGE_INTEGER counter;
			::QueryPerformanceCounter(&counter);

double queryPerformanceCounterTimeNowSec = static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);

baseUnixEpochSec = currentTimeFileTimeSec - queryPerformanceCounterTimeNowSec;

			initialized = true;
		}

		LARGE_INTEGER counter;
		::QueryPerformanceCounter(&counter);

double queryPerformanceCounterTimeSec = static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);

double currentTimeSec = queryPerformanceCounterTimeSec + baseUnixEpochSec;

		Timestamp timestamp;
		timestamp.seconds = currentTimeSec;
		timestamp.milliseconds = static_cast<long long>(currentTimeSec * 1'000.0);
		timestamp.nanoseconds = static_cast<long long>(currentTimeSec * 1'000'000'000.0);

return (timestamp);
	}
}
