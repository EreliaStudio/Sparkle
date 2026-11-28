#include "network/internal/page_failure_limiter.hpp"

#include <gtest/gtest.h>

#include <chrono>

using namespace std::chrono_literals;

TEST(PageFailureLimiter, RequiresTwoPagesAndAtLeast128KiBWithinSixtySeconds)
{
	spk::NetworkInternal::PageFailureLimiter limiter;
	const auto now = spk::NetworkInternal::PageFailureLimiter::Clock::now();
	EXPECT_FALSE(limiter.malformedPage(128u * 1024u, now));
	EXPECT_TRUE(limiter.malformedPage(1u, now + 1ms));
}

TEST(PageFailureLimiter, ByteVolumeWithoutTwoPagesDoesNotTrip)
{
	spk::NetworkInternal::PageFailureLimiter limiter;
	const auto now = spk::NetworkInternal::PageFailureLimiter::Clock::now();
	EXPECT_FALSE(limiter.malformedPage(200u * 1024u, now));
	EXPECT_FALSE(limiter.tripped());
}

TEST(PageFailureLimiter, ExpiresMalformedBytesOutsideRollingWindow)
{
	spk::NetworkInternal::PageFailureLimiter limiter;
	const auto now = spk::NetworkInternal::PageFailureLimiter::Clock::now();
	EXPECT_FALSE(limiter.malformedPage(100u * 1024u, now));
	EXPECT_FALSE(limiter.malformedPage(64u * 1024u, now + 60s));
	EXPECT_FALSE(limiter.tripped());
	EXPECT_TRUE(limiter.malformedPage(64u * 1024u, now + 60s + 1ms));
}

TEST(PageFailureLimiter, TenConsecutiveFailuresTripWithoutByteThreshold)
{
	spk::NetworkInternal::PageFailureLimiter limiter;
	for (std::size_t index = 0; index < 9; ++index)
	{
		EXPECT_FALSE(limiter.failedMessage());
	}
	EXPECT_TRUE(limiter.failedMessage());
}

TEST(PageFailureLimiter, SuccessfulMessageResetsOnlyTheFailureStreak)
{
	spk::NetworkInternal::PageFailureLimiter limiter;
	const auto now = spk::NetworkInternal::PageFailureLimiter::Clock::now();
	EXPECT_FALSE(limiter.malformedPage(64u * 1024u, now));
	for (std::size_t index = 0; index < 9; ++index)
	{
		EXPECT_FALSE(limiter.failedMessage());
	}
	limiter.successfulMessage();
	EXPECT_FALSE(limiter.failedMessage());
	EXPECT_TRUE(limiter.malformedPage(64u * 1024u, now + 1ms));
}

TEST(PageFailureLimiter, ConfigurationIsRuntimeAdjustableAndValidated)
{
	spk::NetworkInternal::PageFailureLimiter limiter;
	spk::NetworkInternal::PageFailureLimiter::Config config;
	config.minimumPages = 3;
	config.minimumBytes = 300;
	config.window = 20s;
	config.consecutiveFailures = 2;
	limiter.configure(config);
	const auto now = spk::NetworkInternal::PageFailureLimiter::Clock::now();
	EXPECT_FALSE(limiter.malformedPage(200, now));
	EXPECT_FALSE(limiter.malformedPage(200, now + 1ms));
	EXPECT_FALSE(limiter.failedMessage());
	EXPECT_TRUE(limiter.failedMessage());
	config.minimumPages = 0;
	EXPECT_THROW(limiter.configure(config), spk::Exception);
	limiter.clear();
	EXPECT_FALSE(limiter.tripped());
}
