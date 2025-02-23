#pragma once

#include <type_traits>

namespace spk
{
	enum class TimeUnit
	{
		Second,
		Millisecond,
		Nanosecond
	};

	struct Timestamp;

	struct Duration
	{
        double seconds = 0;
        long long milliseconds = 0;
        long long nanoseconds = 0;

		Duration();
		Duration(long long p_value, TimeUnit p_unit = TimeUnit::Millisecond);
		Duration(double p_value, TimeUnit p_unit = TimeUnit::Millisecond);

        Duration operator+(const Duration& p_other) const;
        Duration operator-(const Duration& p_other) const;
        Duration& operator+=(const Duration& p_other);
        Duration& operator-=(const Duration& p_other);

        bool operator==(const Duration& p_other) const;
        bool operator!=(const Duration& p_other) const;

        bool operator<(const Duration& p_other) const;
        bool operator>(const Duration& p_other) const;
        bool operator<=(const Duration& p_other) const;
        bool operator>=(const Duration& p_other) const;
	};

	struct Timestamp
    {
        double seconds = 0;
        long long milliseconds = 0;
        long long nanoseconds = 0;

		Timestamp() = default;
		Timestamp(const Duration& d);

		Duration operator - (const Timestamp& p_other) const;

        Timestamp operator+(const Duration& p_other) const;
        Timestamp operator-(const Duration& p_other) const;
        Timestamp& operator+=(const Duration& p_other);
        Timestamp& operator-=(const Duration& p_other);

        bool operator==(const Timestamp& p_other) const;
        bool operator!=(const Timestamp& p_other) const;

        bool operator<(const Timestamp& p_other) const;
        bool operator>(const Timestamp& p_other) const;
        bool operator<=(const Timestamp& p_other) const;
        bool operator>=(const Timestamp& p_other) const;

    private:
        static Timestamp fromNanoseconds(long long ns);
    };
}

namespace spk::SystemUtils
{
	spk::Timestamp getTime();
}