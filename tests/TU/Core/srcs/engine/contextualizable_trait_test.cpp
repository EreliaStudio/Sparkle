#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "engine/contextualizable_trait.hpp"

namespace
{
	TEST(ContextualizableTraitTest, StandardUsageObservesOldAndNewContextsBeforeStorageChanges)
	{
		spk::ContextualizableTrait<int> value(10);
		int observedOld = 0;
		int observedNew = 0;
		int contextVisibleInsideCallback = 0;
		std::size_t callbackCount = 0;

		auto contract = value.subscribeToContextEdition(
			[&](const int &oldContext, const int &newContext)
			{
				++callbackCount;
				observedOld = oldContext;
				observedNew = newContext;
				contextVisibleInsideCallback = value.context();
			});

		value.changeContext(42);

		EXPECT_EQ(callbackCount, 1u);
		EXPECT_EQ(observedOld, 10);
		EXPECT_EQ(observedNew, 42);
		EXPECT_EQ(contextVisibleInsideCallback, 10);
		EXPECT_EQ(value.context(), 42);
	}

	TEST(ContextualizableTraitTest, DefaultConstructedValueContextUsesValueInitialization)
	{
		spk::ContextualizableTrait<int> value;

		EXPECT_EQ(value.context(), 0);
	}

	TEST(ContextualizableTraitTest, SameContextIsANoOp)
	{
		spk::ContextualizableTrait<std::string> value("sparkle");
		std::size_t callbackCount = 0;
		auto contract = value.subscribeToContextEdition(
			[&](const std::string &, const std::string &)
			{
				++callbackCount;
			});

		value.changeContext("sparkle");

		EXPECT_EQ(value.context(), "sparkle");
		EXPECT_EQ(callbackCount, 0u);
	}

	TEST(ContextualizableTraitTest, PointerContextSupportsNullAndNonNullValues)
	{
		int first = 1;
		int second = 2;
		spk::ContextualizableTrait<int *> value;
		std::vector<std::pair<int *, int *>> editions;

		auto contract = value.subscribeToContextEdition(
			[&](int *const &oldContext, int *const &newContext)
			{
				editions.emplace_back(oldContext, newContext);
			});

		EXPECT_EQ(value.context(), nullptr);
		value.changeContext(&first);
		value.changeContext(&second);
		value.changeContext(nullptr);

		ASSERT_EQ(editions.size(), 3u);
		EXPECT_EQ(editions[0], std::make_pair(nullptr, &first));
		EXPECT_EQ(editions[1], std::make_pair(&first, &second));
		EXPECT_EQ(editions[2], std::make_pair(&second, nullptr));
		EXPECT_EQ(value.context(), nullptr);
	}

	TEST(ContextualizableTraitTest, ValueContextsAreStoredByValue)
	{
		std::string initial = "first";
		spk::ContextualizableTrait<std::string> value(initial);
		initial = "modified";

		EXPECT_EQ(value.context(), "first");

		std::string next = "second";
		value.changeContext(next);
		next = "modified again";

		EXPECT_EQ(value.context(), "second");
	}

	TEST(ContextualizableTraitTest, CallbackCanDriveExternalStateFromAnEdition)
	{
		spk::ContextualizableTrait<int> value(3);
		int derivedState = 0;
		auto contract = value.subscribeToContextEdition(
			[&](const int &oldContext, const int &newContext)
			{
				derivedState = newContext - oldContext;
			});

		value.changeContext(11);

		EXPECT_EQ(derivedState, 8);
		EXPECT_EQ(value.context(), 11);
	}

	TEST(ContextualizableTraitTest, ResignedContractNoLongerReceivesEditions)
	{
		spk::ContextualizableTrait<int> value(1);
		std::size_t callbackCount = 0;
		auto contract = value.subscribeToContextEdition(
			[&](const int &, const int &)
			{
				++callbackCount;
			});

		value.changeContext(2);
		ASSERT_EQ(callbackCount, 1u);

		contract.resign();
		value.changeContext(3);

		EXPECT_EQ(callbackCount, 1u);
		EXPECT_EQ(value.context(), 3);
	}
}
