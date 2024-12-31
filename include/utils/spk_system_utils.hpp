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

	struct Duration
	{
		long long value; // Saved as nanosecond

		Duration();
		Duration(long long p_value, TimeUnit p_unit = TimeUnit::Millisecond);
		Duration(double p_value, TimeUnit p_unit = TimeUnit::Millisecond);
	};

	struct Timestamp
    {
        double seconds = 0;
        long long milliseconds = 0;
        long long nanoseconds = 0;

		Timestamp() = default;
		Timestamp(const Duration& d);

        Timestamp operator+(const Timestamp& other) const;
        Timestamp operator-(const Timestamp& other) const;
        Timestamp& operator+=(const Timestamp& other);
        Timestamp& operator-=(const Timestamp& other);

        bool operator==(const Timestamp& other) const;
        bool operator!=(const Timestamp& other) const;

        bool operator<(const Timestamp& other) const;
        bool operator>(const Timestamp& other) const;
        bool operator<=(const Timestamp& other) const;
        bool operator>=(const Timestamp& other) const;

    private:
        static Timestamp fromNanoseconds(long long ns);
    };
}

namespace spk::SystemUtils
{
	spk::Timestamp getTime();
}