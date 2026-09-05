#include <gtest/gtest.h>

#include "design_pattern/trait/statefull_trait.hpp"

#include <optional>
#include <string>

namespace
{
	enum class State
	{
		Idle,
		Running,
		Paused
	};
}

TEST(StatefullTraitTest, StandardUsageFiresOnlyMatchingStateAfterStoredStateChanges)
{
	spk::StatefullTrait<State> trait;
	int idleCount = 0;
	int runningCount = 0;
	bool callbackSawUpdatedState = false;

	auto idleContract = trait.subscribeToStateEnter(State::Idle, [&]() {
		++idleCount;
	});
	auto runningContract = trait.subscribeToStateEnter(State::Running, [&]() {
		++runningCount;
		callbackSawUpdatedState =
			trait.state().has_value() &&
			*trait.state() == State::Running;
	});

	trait.setState(State::Running);

	ASSERT_TRUE(trait.state().has_value());
	EXPECT_EQ(*trait.state(), State::Running);
	EXPECT_EQ(idleCount, 0);
	EXPECT_EQ(runningCount, 1);
	EXPECT_TRUE(callbackSawUpdatedState);
}

TEST(StatefullTraitTest, InitialStateIsUnset)
{
	const spk::StatefullTrait<State> trait;

	EXPECT_FALSE(trait.state().has_value());
}

TEST(StatefullTraitTest, SettingSameStateIsANoOp)
{
	spk::StatefullTrait<State> trait;
	int callCount = 0;

	auto contract = trait.subscribeToStateEnter(State::Idle, [&]() {
		++callCount;
	});

	trait.setState(State::Idle);
	trait.setState(State::Idle);
	trait.setState(State::Idle);

	EXPECT_EQ(callCount, 1);
	ASSERT_TRUE(trait.state().has_value());
	EXPECT_EQ(*trait.state(), State::Idle);
}

TEST(StatefullTraitTest, SeveralStatesHaveIndependentProviders)
{
	spk::StatefullTrait<State> trait;
	int idleCount = 0;
	int runningCount = 0;
	int pausedCount = 0;

	auto idle = trait.subscribeToStateEnter(State::Idle, [&]() { ++idleCount; });
	auto running = trait.subscribeToStateEnter(State::Running, [&]() { ++runningCount; });
	auto paused = trait.subscribeToStateEnter(State::Paused, [&]() { ++pausedCount; });

	trait.setState(State::Idle);
	trait.setState(State::Running);
	trait.setState(State::Paused);
	trait.setState(State::Running);

	EXPECT_EQ(idleCount, 1);
	EXPECT_EQ(runningCount, 2);
	EXPECT_EQ(pausedCount, 1);
}

TEST(StatefullTraitTest, HashableNonEnumStateTypesAreSupported)
{
	spk::StatefullTrait<std::string> trait;
	int readyCount = 0;

	auto contract = trait.subscribeToStateEnter("ready", [&]() {
		++readyCount;
	});

	trait.setState("loading");
	trait.setState("ready");

	ASSERT_TRUE(trait.state().has_value());
	EXPECT_EQ(*trait.state(), "ready");
	EXPECT_EQ(readyCount, 1);
}

TEST(StatefullTraitTest, ContractResignationStopsStateNotifications)
{
	spk::StatefullTrait<State> trait;
	int count = 0;

	auto contract = trait.subscribeToStateEnter(State::Running, [&]() {
		++count;
	});

	trait.setState(State::Running);
	EXPECT_EQ(count, 1);

	contract.resign();
	trait.setState(State::Idle);
	trait.setState(State::Running);

	EXPECT_EQ(count, 1);
}

TEST(StatefullTraitTest, CallbackDrivenStateChangeUpdatesToFinalStateAndFiresNestedProvider)
{
	spk::StatefullTrait<State> trait;
	int runningCount = 0;
	int pausedCount = 0;

	auto paused = trait.subscribeToStateEnter(State::Paused, [&]() {
		++pausedCount;
	});
	auto running = trait.subscribeToStateEnter(State::Running, [&]() {
		++runningCount;
		trait.setState(State::Paused);
	});

	trait.setState(State::Running);

	EXPECT_EQ(runningCount, 1);
	EXPECT_EQ(pausedCount, 1);
	ASSERT_TRUE(trait.state().has_value());
	EXPECT_EQ(*trait.state(), State::Paused);
}
