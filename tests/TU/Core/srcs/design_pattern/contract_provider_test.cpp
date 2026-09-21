#include <gtest/gtest.h>

#include "design_pattern/contract_provider.hpp"

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

TEST(ContractProviderTest, StandardUsageTriggersInOrderResignsAndUsesRAIIUnsubscription)
{
	spk::ContractProvider<int> provider;
	std::vector<std::string> calls;

	auto first = provider.subscribe([&](int value) {
		calls.push_back("first:" + std::to_string(value));
	});
	auto second = provider.subscribe([&](int value) {
		calls.push_back("second:" + std::to_string(value));
	});
	{
		auto third = provider.subscribe([&](int value) {
			calls.push_back("third:" + std::to_string(value));
		});

		provider.trigger(1);
		EXPECT_EQ(
			calls,
			(std::vector<std::string>{"first:1", "second:1", "third:1"}));

		second.resign();
		calls.clear();

		provider.trigger(2);
		EXPECT_EQ(
			calls,
			(std::vector<std::string>{"first:2", "third:2"}));
	}

	calls.clear();
	provider.trigger(3);
	EXPECT_EQ(calls, (std::vector<std::string>{"first:3"}));
}

TEST(ContractProviderTest, EmptyProviderIsSafeAndReportsEmpty)
{
	spk::ContractProvider<> provider;

	EXPECT_TRUE(provider.empty());
	EXPECT_NO_THROW(provider.trigger());
	EXPECT_TRUE(provider.empty());
}

TEST(ContractProviderTest, EmptyCallbackFailureLeavesProviderUsable)
{
	spk::ContractProvider<> provider;
	auto emptyContract = provider.subscribe(std::function<void()>{});

	EXPECT_TRUE(emptyContract.isValid());
	EXPECT_THROW(provider.trigger(), std::bad_function_call);

	emptyContract.resign();

	int callCount = 0;
	auto validContract = provider.subscribe([&]() {
		++callCount;
	});

	EXPECT_NO_THROW(provider.trigger());
	EXPECT_EQ(callCount, 1);
	EXPECT_TRUE(validContract.isValid());
}

TEST(ContractProviderTest, ContractsMoveAndSelfMoveWithoutDuplicatingRegistration)
{
	spk::ContractProvider<> provider;
	int callCount = 0;

	auto first = provider.subscribe([&]() {
		++callCount;
	});
	ASSERT_TRUE(first.isValid());

	auto moved = std::move(first);
	EXPECT_FALSE(first.isValid());
	EXPECT_TRUE(moved.isValid());

	provider.trigger();
	EXPECT_EQ(callCount, 1);

	moved = std::move(moved);
	EXPECT_TRUE(moved.isValid());

	provider.trigger();
	EXPECT_EQ(callCount, 2);

	spk::ContractProvider<>::Contract assigned;
	assigned = std::move(moved);
	EXPECT_FALSE(moved.isValid());
	EXPECT_TRUE(assigned.isValid());

	provider.trigger();
	EXPECT_EQ(callCount, 3);
}

TEST(ContractProviderTest, ProviderDestructionInvalidatesOutstandingContracts)
{
	spk::ContractProvider<>::Contract contract;

	{
		spk::ContractProvider<> provider;
		contract = provider.subscribe([]() {});
		ASSERT_TRUE(contract.isValid());
	}

	EXPECT_FALSE(contract.isValid());
	EXPECT_NO_THROW(contract.resign());
}

TEST(ContractProviderTest, ExplicitInvalidationInvalidatesAllContractsAndEmptiesProvider)
{
	spk::ContractProvider<> provider;

	auto first = provider.subscribe([]() {});
	auto second = provider.subscribe([]() {});
	ASSERT_FALSE(provider.empty());

	provider.invalidate();

	EXPECT_TRUE(provider.empty());
	EXPECT_FALSE(first.isValid());
	EXPECT_FALSE(second.isValid());
	EXPECT_NO_THROW(provider.trigger());
}

TEST(ContractProviderTest, SubscriptionDuringDispatchIsDeferredUntilNextTrigger)
{
	spk::ContractProvider<int> provider;
	std::vector<int> calls;
	spk::ContractProvider<int>::Contract lateContract;

	auto first = provider.subscribe([&](int value) {
		calls.push_back(value * 10 + 1);
		if (!lateContract)
		{
			lateContract = provider.subscribe([&](int nestedValue) {
				calls.push_back(nestedValue * 10 + 3);
			});
		}
	});

	auto second = provider.subscribe([&](int value) {
		calls.push_back(value * 10 + 2);
	});

	provider.trigger(1);
	EXPECT_EQ(calls, (std::vector<int>{11, 12}));

	calls.clear();
	provider.trigger(2);
	EXPECT_EQ(calls, (std::vector<int>{21, 22, 23}));
}

TEST(ContractProviderTest, ResignationDuringDispatchPreventsLaterCallbackAndIsAppliedAfterDispatch)
{
	spk::ContractProvider<> provider;
	std::vector<int> calls;
	spk::ContractProvider<>::Contract second;

	auto first = provider.subscribe([&]() {
		calls.push_back(1);
		second.resign();
	});
	second = provider.subscribe([&]() {
		calls.push_back(2);
	});
	auto third = provider.subscribe([&]() {
		calls.push_back(3);
	});

	provider.trigger();
	EXPECT_EQ(calls, (std::vector<int>{1, 3}));
	EXPECT_FALSE(second.isValid());

	calls.clear();
	provider.trigger();
	EXPECT_EQ(calls, (std::vector<int>{1, 3}));
}

TEST(ContractProviderTest, InvalidationDuringDispatchStopsRemainingCallbacks)
{
	spk::ContractProvider<> provider;
	std::vector<int> calls;

	auto first = provider.subscribe([&]() {
		calls.push_back(1);
		provider.invalidate();
	});
	auto second = provider.subscribe([&]() {
		calls.push_back(2);
	});

	provider.trigger();

	EXPECT_EQ(calls, (std::vector<int>{1}));
	EXPECT_TRUE(provider.empty());
	EXPECT_FALSE(first.isValid());
	EXPECT_FALSE(second.isValid());
}

TEST(ContractProviderTest, NestedTriggersUseLatestQueuedArgumentsAndPreserveOrder)
{
	spk::ContractProvider<int> provider;
	std::vector<int> firstCalls;
	std::vector<int> secondCalls;

	auto first = provider.subscribe([&](int value) {
		firstCalls.push_back(value);
		if (value == 1)
		{
			provider.trigger(2);
			provider.trigger(3);
		}
	});

	auto second = provider.subscribe([&](int value) {
		secondCalls.push_back(value);
	});

	provider.trigger(1);

	EXPECT_EQ(firstCalls, (std::vector<int>{1, 3}));
	EXPECT_EQ(secondCalls, (std::vector<int>{1, 3}));
}

TEST(ContractProviderTest, ThrowingCallbackRestoresProviderAndAppliesPendingRemoval)
{
	spk::ContractProvider<> provider;
	bool throwOnFirstCall = true;
	int firstCalls = 0;
	int secondCalls = 0;

	spk::ContractProvider<>::Contract second;
	auto first = provider.subscribe([&]() {
		++firstCalls;
		second.resign();
		if (throwOnFirstCall)
		{
			throwOnFirstCall = false;
			throw std::runtime_error("callback failure");
		}
	});
	second = provider.subscribe([&]() {
		++secondCalls;
	});

	try
	{
		provider.trigger();
		FAIL() << "Expected callback exception";
	}
	catch (const std::runtime_error &exception)
	{
		EXPECT_STREQ(exception.what(), "callback failure");
	}

	EXPECT_EQ(firstCalls, 1);
	EXPECT_EQ(secondCalls, 0);
	EXPECT_FALSE(second.isValid());

	EXPECT_NO_THROW(provider.trigger());
	EXPECT_EQ(firstCalls, 2);
	EXPECT_EQ(secondCalls, 0);
}
