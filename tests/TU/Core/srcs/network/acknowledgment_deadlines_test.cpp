#include "network/internal/acknowledgment_deadlines.hpp"

#include "exception.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace
{
	using spk::NetworkInternal::AcknowledgmentDeadlines;
}

TEST(AcknowledgmentDeadlines, StartsOnlyWhenExplicitlyMarkedWritten)
{
	AcknowledgmentDeadlines deadlines;
	const auto now = AcknowledgmentDeadlines::Clock::now();
	EXPECT_TRUE(deadlines.expire(now + std::chrono::minutes(5)).empty());
	deadlines.start(14, now);
	EXPECT_TRUE(deadlines.expire(now + std::chrono::seconds(29)).empty());
	EXPECT_EQ(deadlines.expire(now + std::chrono::seconds(30)), (std::vector<std::uint64_t>{14}));
	EXPECT_EQ(deadlines.pending(), 0u);
}

TEST(AcknowledgmentDeadlines, RuntimeChangeAffectsFutureWritesOnly)
{
	AcknowledgmentDeadlines deadlines;
	const auto now = AcknowledgmentDeadlines::Clock::now();
	deadlines.start(1, now);
	deadlines.setTimeout(std::chrono::milliseconds(100));
	deadlines.start(2, now);
	EXPECT_EQ(deadlines.expire(now + std::chrono::milliseconds(100)), (std::vector<std::uint64_t>{2}));
	EXPECT_EQ(deadlines.expire(now + std::chrono::seconds(30)), (std::vector<std::uint64_t>{1}));
}

TEST(AcknowledgmentDeadlines, RepeatedMarkDoesNotRestartDeadline)
{
	AcknowledgmentDeadlines deadlines;
	const auto now = AcknowledgmentDeadlines::Clock::now();
	deadlines.start(3, now);
	deadlines.start(3, now + std::chrono::seconds(20));
	EXPECT_EQ(deadlines.expire(now + std::chrono::seconds(30)), (std::vector<std::uint64_t>{3}));
}

TEST(AcknowledgmentDeadlines, AcknowledgmentCancelsTimeout)
{
	AcknowledgmentDeadlines deadlines;
	const auto now = AcknowledgmentDeadlines::Clock::now();
	deadlines.start(1, now);
	deadlines.forget(1);
	EXPECT_TRUE(deadlines.expire(now + std::chrono::minutes(1)).empty());
	EXPECT_THROW(deadlines.setTimeout(std::chrono::milliseconds(0)), spk::Exception);
	EXPECT_THROW(deadlines.start(0), spk::Exception);
	deadlines.start(2, now);
	deadlines.clear();
	EXPECT_EQ(deadlines.pending(), 0u);
}
