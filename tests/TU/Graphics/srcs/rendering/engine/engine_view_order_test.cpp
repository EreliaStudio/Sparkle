#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "exception.hpp"
#include "engine_view_test_utils.hpp"

namespace
{
	using Fixture = sparkle_test::engine_view::Fixture;
	using Order = spk::EngineView::Order;

	std::vector<std::string> capture(Fixture &fixture)
	{
		fixture.engine.refreshRenderSnapshot();
		fixture.executeLatest();
		return fixture.log;
	}
}

TEST(EngineViewOrderTest, UnorderedViewsFollowCreationOrder)
{
	Fixture fixture;
	fixture.addView("zeta");
	fixture.addView("alpha");
	fixture.addView("middle");
	fixture.record("value");
	EXPECT_EQ(capture(fixture), (std::vector<std::string>{"zeta:value", "alpha:value", "middle:value"}));
}

TEST(EngineViewOrderTest, ExplicitOrdersAreSortedNumerically)
{
	Fixture fixture;
	fixture.addView("late", 90);
	fixture.addView("early", -4);
	fixture.addView("middle", 0);
	fixture.record("value");
	EXPECT_EQ(capture(fixture), (std::vector<std::string>{"early:value", "middle:value", "late:value"}));
}

TEST(EngineViewOrderTest, ExplicitViewsAlwaysPrecedeUnorderedViews)
{
	Fixture fixture;
	fixture.addView("unordered-first");
	fixture.addView("explicit-middle", 4);
	fixture.addView("unordered-second");
	fixture.addView("explicit-first", -2);
	fixture.record("value");
	EXPECT_EQ(capture(fixture), (std::vector<std::string>{
		"explicit-first:value", "explicit-middle:value",
		"unordered-first:value", "unordered-second:value"}));
}

TEST(EngineViewOrderTest, LatePriorityInsertionPrecedesEarlierUnorderedView)
{
	Fixture fixture;
	fixture.addView("ordinary");
	fixture.addView("priority", -1);
	fixture.record("value");
	EXPECT_EQ(capture(fixture), (std::vector<std::string>{"priority:value", "ordinary:value"}));
}

TEST(EngineViewOrderTest, ChangingOrderReordersFollowingCapture)
{
	Fixture fixture;
	fixture.addView("first", 0);
	auto &second = fixture.addView("second");
	fixture.record("value");
	EXPECT_EQ(capture(fixture), (std::vector<std::string>{"first:value", "second:value"}));
	fixture.log.clear();
	second.setOrder(-1);
	EXPECT_EQ(capture(fixture), (std::vector<std::string>{"second:value", "first:value"}));
}

TEST(EngineViewOrderTest, ClearingAnOrderRestoresUnorderedCreationSequence)
{
	Fixture fixture;
	auto &first = fixture.addView("first", 9);
	fixture.addView("second");
	fixture.addView("third");
	first.setOrder(std::nullopt);
	fixture.record("value");
	EXPECT_EQ(capture(fixture), (std::vector<std::string>{"first:value", "second:value", "third:value"}));
}

TEST(EngineViewOrderTest, DuplicateOrderAtCreationHasStrongGuarantee)
{
	Fixture fixture;
	auto &first = fixture.addView("first", 7);
	EXPECT_THROW(fixture.addView("other", 7), spk::Exception);
	EXPECT_EQ(first.order(), std::optional<Order>{7});
	EXPECT_THROW(fixture.engine.view("other"), spk::Exception);
}

TEST(EngineViewOrderTest, DuplicateOrderAtMutationDoesNotChangeEitherView)
{
	Fixture fixture;
	auto &first = fixture.addView("first", 7);
	auto &second = fixture.addView("second", 8);
	EXPECT_THROW(second.setOrder(7), spk::Exception);
	EXPECT_EQ(first.order(), std::optional<Order>{7});
	EXPECT_EQ(second.order(), std::optional<Order>{8});
}

TEST(EngineViewOrderTest, RemovedViewReleasesItsOrder)
{
	Fixture fixture;
	fixture.addView("first", 7);
	fixture.engine.removeView("first");
	auto &replacement = fixture.addView("replacement", 7);
	EXPECT_EQ(replacement.order(), std::optional<Order>{7});
}

TEST(EngineViewOrderTest, Int32MinAndMaxCanBeUsedTogether)
{
	Fixture fixture;
	fixture.addView("unordered");
	fixture.addView("maximum", std::numeric_limits<Order>::max());
	fixture.addView("minimum", std::numeric_limits<Order>::min());
	fixture.record("value");
	EXPECT_EQ(capture(fixture), (std::vector<std::string>{
		"minimum:value", "maximum:value", "unordered:value"}));
}

TEST(EngineViewOrderTest, HighestOrderRemainsBeforeUnorderedView)
{
	Fixture fixture;
	fixture.addView("unordered");
	fixture.addView("last-explicit", std::numeric_limits<Order>::max());
	fixture.record("value");
	EXPECT_EQ(capture(fixture), (std::vector<std::string>{"last-explicit:value", "unordered:value"}));
}

TEST(EngineViewOrderTest, DetachedViewProducesNoNewSubSnapshotOrGap)
{
	Fixture fixture;
	fixture.addView("first", -10);
	auto &detached = fixture.addView("middle", -5);
	fixture.addView("last", 0);
	fixture.record("value");
	detached.detachCamera();
	EXPECT_EQ(capture(fixture), (std::vector<std::string>{"first:value", "last:value"}));
}
