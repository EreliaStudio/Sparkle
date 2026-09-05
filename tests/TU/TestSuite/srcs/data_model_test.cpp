#include <gtest/gtest.h>

#include <concepts>
#include "container/data_model.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
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

TEST(DataModelTest, StandardUsageEditsRowsAndReportsExactNotificationRanges)
{
	spk::DataModel<std::string> model({"alpha", "gamma"});
	std::vector<RangeEvent> insertedEvents;
	std::vector<RangeEvent> changedEvents;
	std::vector<RangeEvent> removedEvents;
	int resetCount = 0;

	auto insertedContract = model.subscribeToRowsInserted(
		[&insertedEvents](std::size_t first, std::size_t count) {
			insertedEvents.push_back({first, count});
		});
	auto changedContract = model.subscribeToRowsChanged(
		[&changedEvents](std::size_t first, std::size_t count) {
			changedEvents.push_back({first, count});
		});
	auto removedContract = model.subscribeToRowsRemoved(
		[&removedEvents](std::size_t first, std::size_t count) {
			removedEvents.push_back({first, count});
		});
	auto resetContract = model.subscribeToReset([&resetCount]() { ++resetCount; });
	(void)insertedContract;
	(void)changedContract;
	(void)removedContract;
	(void)resetContract;

	const auto alphaID = model.rowID(0);
	const auto gammaID = model.rowID(1);
	const auto betaID = model.insert(1, "beta");
	EXPECT_EQ((std::vector<RangeEvent>{{1, 1}}), insertedEvents);
	EXPECT_EQ(model.data(1), "beta");
	EXPECT_EQ(model.rowFromID(alphaID), 0u);
	EXPECT_EQ(model.rowFromID(betaID), 1u);
	EXPECT_EQ(model.rowFromID(gammaID), 2u);

	model.setData(1, "BETA");
	model.setEnabled(1, false);
	EXPECT_EQ((std::vector<RangeEvent>{{1, 1}, {1, 1}}), changedEvents);
	EXPECT_EQ(model.data(1), "BETA");
	EXPECT_FALSE(model.isEnabled(1));

	model.erase(0, 2);
	EXPECT_EQ((std::vector<RangeEvent>{{0, 2}}), removedEvents);
	EXPECT_FALSE(model.rowFromID(alphaID).has_value());
	EXPECT_FALSE(model.rowFromID(betaID).has_value());
	EXPECT_EQ(model.rowFromID(gammaID), 0u);

	model.clear();
	EXPECT_TRUE(model.empty());
	EXPECT_EQ(resetCount, 1);
	model.clear();
	EXPECT_EQ(resetCount, 1);
}

TEST(DataModelTest, EmptyModelSupportsNoOpClearAndZeroCountErase)
{
	spk::DataModel<int> model;
	int resetCount = 0;
	int removedCount = 0;
	auto resetContract = model.subscribeToReset([&resetCount]() { ++resetCount; });
	auto removedContract = model.subscribeToRowsRemoved(
		[&removedCount](std::size_t, std::size_t) { ++removedCount; });
	(void)resetContract;
	(void)removedContract;

	EXPECT_TRUE(model.empty());
	EXPECT_EQ(model.rowCount(), 0u);
	model.clear();
	model.erase(0, 0);
	EXPECT_EQ(resetCount, 0);
	EXPECT_EQ(removedCount, 0);
}

TEST(DataModelTest, InitializerListAndVectorConstructionPreserveContents)
{
	spk::DataModel<int> initializerModel({1, 2, 3});
	ASSERT_EQ(initializerModel.rowCount(), 3u);
	EXPECT_EQ(initializerModel.data(0), 1);
	EXPECT_EQ(initializerModel.data(1), 2);
	EXPECT_EQ(initializerModel.data(2), 3);

	std::vector<std::string> contents{"a", "b", "c"};
	spk::DataModel<std::string> vectorModel(std::move(contents));
	ASSERT_EQ(vectorModel.rowCount(), 3u);
	EXPECT_EQ(vectorModel.data(0), "a");
	EXPECT_EQ(vectorModel.data(2), "c");
}

TEST(DataModelTest, InsertionsAtFrontMiddleAndEndPreserveStableRowIDs)
{
	spk::DataModel<std::string> model({"middle"});
	const auto middleID = model.rowID(0);
	const auto frontID = model.emplace(0, "front");
	const auto endID = model.emplaceBack("end");
	const auto betweenID = model.emplace(2, "between");

	EXPECT_EQ(model.data(0), "front");
	EXPECT_EQ(model.data(1), "middle");
	EXPECT_EQ(model.data(2), "between");
	EXPECT_EQ(model.data(3), "end");
	EXPECT_EQ(model.rowFromID(frontID), 0u);
	EXPECT_EQ(model.rowFromID(middleID), 1u);
	EXPECT_EQ(model.rowFromID(betweenID), 2u);
	EXPECT_EQ(model.rowFromID(endID), 3u);
}

TEST(DataModelTest, RowIDsDisappearAfterEraseAndAreNotReusedAfterClear)
{
	spk::DataModel<int> model;
	const auto first = model.append(1);
	const auto second = model.append(2);
	model.erase(0);
	EXPECT_FALSE(model.rowFromID(first).has_value());
	EXPECT_EQ(model.rowFromID(second), 0u);

	model.clear();
	EXPECT_FALSE(model.rowFromID(second).has_value());
	const auto afterReset = model.append(3);
	EXPECT_GT(afterReset, second);
}

TEST(DataModelTest, RepeatedNoOpEnableDoesNotNotify)
{
	spk::DataModel<int> model({1});
	int changedCount = 0;
	auto contract = model.subscribeToRowsChanged(
		[&changedCount](std::size_t, std::size_t) { ++changedCount; });
	(void)contract;

	model.setEnabled(0, true);
	EXPECT_EQ(changedCount, 0);
	model.setEnabled(0, false);
	EXPECT_EQ(changedCount, 1);
	model.setEnabled(0, false);
	EXPECT_EQ(changedCount, 1);
}

TEST(DataModelTest, ConstAccessorsExposeRowsWithoutMutation)
{
	const spk::DataModel<std::string> model({"immutable"});
	static_assert(std::same_as<decltype(model.data(0)), const std::string &>);
	EXPECT_EQ(model.rowCount(), 1u);
	EXPECT_FALSE(model.empty());
	EXPECT_EQ(model.data(0), "immutable");
	EXPECT_TRUE(model.isEnabled(0));
	EXPECT_TRUE(model.rowFromID(model.rowID(0)).has_value());
}

TEST(DataModelTest, InvalidRowAccessInsertionAndEraseRangesThrowOutOfRange)
{
	spk::DataModel<int> model({1, 2});
	EXPECT_THROW((void)model.data(2), std::out_of_range);
	EXPECT_THROW((void)model.rowID(2), std::out_of_range);
	EXPECT_THROW((void)model.isEnabled(2), std::out_of_range);
	EXPECT_THROW(model.setData(2, 3), std::out_of_range);
	EXPECT_THROW(model.setEnabled(2, false), std::out_of_range);
	EXPECT_THROW((void)model.insert(3, 3), std::out_of_range);
	EXPECT_THROW(model.erase(3, 0), std::out_of_range);
	EXPECT_THROW(model.erase(1, 2), std::out_of_range);
}

TEST(DataModelTest, DISABLED_RowIDOverflowRequiresDeterministicPublicTestSeam)
{
	GTEST_SKIP() << "_nextRowID is private and the supplied API exposes no deterministic seam for forcing uint64_t exhaustion.";
}

TEST(DataModelTest, DISABLED_MutableDataAccessorIsNotPresentInSuppliedPublicAPI)
{
	GTEST_SKIP() << "data(std::size_t) is const-only in the supplied DataModel snapshot; a mutable accessor cannot be tested without inventing an API.";
}
