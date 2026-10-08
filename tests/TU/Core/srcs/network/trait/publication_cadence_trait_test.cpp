#include <exception.hpp>
#include <gtest/gtest.h>
#include <network/trait/publication_cadence_trait.hpp>
using namespace std::chrono_literals;

class PublicationCadenceTraitTest : public testing::Test
{
protected:
	class Cadence : public spk::Network::PublicationCadenceTrait
	{
	public:
		using PublicationCadenceTrait::_advancePublication;
		using PublicationCadenceTrait::_publicationDue;
		using PublicationCadenceTrait::PublicationCadenceTrait;
	};
	spk::Network::Clock::time_point now{};
};

TEST_F(PublicationCadenceTraitTest, DueChecksDoNotConsumeThePassAndLatePassesStartANewInterval)
{
	Cadence cadence(50ms);
	EXPECT_TRUE(cadence._publicationDue(now));
	EXPECT_TRUE(cadence._publicationDue(now));
	cadence._advancePublication(now);
	EXPECT_FALSE(cadence._publicationDue(now + 49ms));
	EXPECT_TRUE(cadence._publicationDue(now + 50ms));
	cadence._advancePublication(now + 120ms);
	EXPECT_FALSE(cadence._publicationDue(now + 169ms));
	EXPECT_TRUE(cadence._publicationDue(now + 170ms));
}

TEST_F(PublicationCadenceTraitTest, ZeroIntervalAllowsEveryPassAndNegativeIntervalIsRejected)
{
	Cadence cadence(0ms);
	cadence._advancePublication(now);
	EXPECT_TRUE(cadence._publicationDue(now));
	EXPECT_THROW(Cadence(-1ms), spk::Exception);
}
