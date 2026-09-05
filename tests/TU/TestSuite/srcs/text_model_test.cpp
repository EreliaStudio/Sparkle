#include <gtest/gtest.h>

#include "container/text_model.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace
{
	struct RangeEvent
	{
		std::size_t first;
		std::size_t count;
		bool operator==(const RangeEvent &) const = default;
	};
}

TEST(TextModelTest, StandardUsageUsesInheritedStringModelAPI)
{
	spk::TextModel model({"first", "second"});
	ASSERT_EQ(model.rowCount(), 2u);
	EXPECT_EQ(model.data(0), "first");
	EXPECT_EQ(model.data(1), "second");

	const auto insertedID = model.append("third");
	EXPECT_EQ(model.rowFromID(insertedID), 2u);
	model.setData(2, "THIRD");
	model.setEnabled(2, false);
	EXPECT_EQ(model.data(2), "THIRD");
	EXPECT_FALSE(model.isEnabled(2));
}

TEST(TextModelTest, Utf8EmptyAndDuplicateStringsArePreserved)
{
	spk::TextModel model({"héllø 世界", "", "duplicate", "duplicate"});
	ASSERT_EQ(model.rowCount(), 4u);
	EXPECT_EQ(model.data(0), "héllø 世界");
	EXPECT_TRUE(model.data(1).empty());
	EXPECT_EQ(model.data(2), "duplicate");
	EXPECT_EQ(model.data(3), "duplicate");
	EXPECT_NE(model.rowID(2), model.rowID(3));
}

TEST(TextModelTest, StableIDsAndInheritedNotificationsMatchDataModelBehavior)
{
	spk::TextModel model({"a", "c"});
	std::vector<RangeEvent> inserted;
	std::vector<RangeEvent> changed;
	std::vector<RangeEvent> removed;
	int resets = 0;

	auto insertedContract = model.subscribeToRowsInserted(
		[&inserted](std::size_t first, std::size_t count) { inserted.push_back({first, count}); });
	auto changedContract = model.subscribeToRowsChanged(
		[&changed](std::size_t first, std::size_t count) { changed.push_back({first, count}); });
	auto removedContract = model.subscribeToRowsRemoved(
		[&removed](std::size_t first, std::size_t count) { removed.push_back({first, count}); });
	auto resetContract = model.subscribeToReset([&resets]() { ++resets; });
	(void)insertedContract;
	(void)changedContract;
	(void)removedContract;
	(void)resetContract;

	const auto aID = model.rowID(0);
	const auto cID = model.rowID(1);
	const auto bID = model.insert(1, "b");
	model.setEnabled(1, false);
	model.erase(0);

	EXPECT_EQ((std::vector<RangeEvent>{{1, 1}}), inserted);
	EXPECT_EQ((std::vector<RangeEvent>{{1, 1}}), changed);
	EXPECT_EQ((std::vector<RangeEvent>{{0, 1}}), removed);
	EXPECT_FALSE(model.rowFromID(aID).has_value());
	EXPECT_EQ(model.rowFromID(bID), 0u);
	EXPECT_EQ(model.rowFromID(cID), 1u);

	model.clear();
	EXPECT_EQ(resets, 1);
}

TEST(TextModelTest, DISABLED_ViewAndDefaultDelegateRequireTheirDefinitions)
{
	GTEST_SKIP() << "TextModel::View and TextModel::Delegate are only forward-declared in the supplied snapshot; their construction/binding API is not available here.";
}
