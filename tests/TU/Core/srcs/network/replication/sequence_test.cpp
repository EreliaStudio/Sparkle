#include <gtest/gtest.h>
#include <network/replication/sequence.hpp>
TEST(NetworkSequenceTest, AdvancesFromInitialValue)
{
	spk::Network::Sequence sequence(10);
	EXPECT_EQ(sequence.value(), 10u);
	EXPECT_EQ(sequence.next(), 11u);
	EXPECT_EQ(sequence.next(), 12u);
}
TEST(NetworkSequenceTest, ExhaustionDoesNotWrap)
{
	const auto maximum = std::numeric_limits<std::uint64_t>::max();
	spk::Network::Sequence sequence(maximum - 1);
	EXPECT_EQ(sequence.next(), maximum);
	EXPECT_THROW((void)sequence.next(), spk::Exception);
	EXPECT_EQ(sequence.value(), maximum);
}
