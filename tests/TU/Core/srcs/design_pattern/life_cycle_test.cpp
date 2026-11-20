#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "design_pattern/life_cycle.hpp"

namespace
{
	struct LifeCycleProbe : spk::LifeCycle
	{
		using LifeCycle::acquire;
		using LifeCycle::finalizeLifeCycle;
		using LifeCycle::release;
	};
}

TEST(LifeCycleTest, StartsAcquiredWithoutEmittingAcquisition)
{
	LifeCycleProbe probe;
	int acquired = 0;
	auto contract = probe.subscribeToAcquisition([&]() {
		++acquired;
	});

	EXPECT_EQ(probe.state(), spk::LifeCycle::State::Acquired);
	EXPECT_TRUE(probe.isAcquired());
	EXPECT_FALSE(probe.isReleased());
	EXPECT_EQ(acquired, 0);

	probe.acquire();
	EXPECT_EQ(acquired, 0);
}

TEST(LifeCycleTest, TransitionsAreIdempotentAndSubscriptionsAreDistinct)
{
	LifeCycleProbe probe;
	std::vector<std::string> calls;
	auto acquisition = probe.subscribeToAcquisition([&]() {
		calls.push_back("acquisition");
	});
	auto release = probe.subscribeToRelease([&]() {
		calls.push_back("release");
	});
	auto destruction = probe.subscribeToDestruction([&]() {
		calls.push_back("destruction");
	});

	probe.release();
	probe.release();
	EXPECT_TRUE(probe.isReleased());
	probe.acquire();
	probe.acquire();
	EXPECT_TRUE(probe.isAcquired());
	probe.release();
	EXPECT_EQ(calls, (std::vector<std::string>{"release", "acquisition", "release"}));

	probe.finalizeLifeCycle();
	probe.finalizeLifeCycle();
	EXPECT_EQ(calls, (std::vector<std::string>{"release", "acquisition", "release", "destruction"}));
}

TEST(LifeCycleTest, DestructionReleasesBeforeNotifyingDestruction)
{
	std::vector<std::string> calls;
	auto object = std::make_unique<LifeCycleProbe>();
	auto release = object->subscribeToRelease([&]() {
		calls.push_back("release");
	});
	auto destruction = object->subscribeToDestruction([&]() {
		calls.push_back("destruction");
	});

	object.reset();
	EXPECT_EQ(calls, (std::vector<std::string>{"release", "destruction"}));
	EXPECT_FALSE(release.isValid());
	EXPECT_FALSE(destruction.isValid());
}

TEST(LifeCycleTest, DestructionOfReleasedObjectDoesNotReleaseTwice)
{
	std::vector<std::string> calls;
	auto object = std::make_unique<LifeCycleProbe>();
	auto release = object->subscribeToRelease([&]() {
		calls.push_back("release");
	});
	auto destruction = object->subscribeToDestruction([&]() {
		calls.push_back("destruction");
	});

	object->release();
	object.reset();
	EXPECT_EQ(calls, (std::vector<std::string>{"release", "destruction"}));
}

TEST(LifeCycleTest, SubscriberFailuresCannotPreventDestruction)
{
	int destructionCount = 0;
	auto object = std::make_unique<LifeCycleProbe>();
	auto release = object->subscribeToRelease([]() {
		throw std::runtime_error("release failure");
	});
	auto destruction = object->subscribeToDestruction([&]() {
		++destructionCount;
		throw std::runtime_error("destruction failure");
	});

	EXPECT_NO_THROW(object.reset());
	EXPECT_EQ(destructionCount, 1);
}

TEST(LifeCycleTest, ResignationDuringDispatchPreventsLaterCallback)
{
	LifeCycleProbe probe;
	int firstCount = 0;
	int secondCount = 0;
	spk::LifeCycle::ReleaseContract second;
	auto first = probe.subscribeToRelease([&]() {
		++firstCount;
		second.resign();
	});
	second = probe.subscribeToRelease([&]() {
		++secondCount;
	});

	probe.release();
	probe.acquire();
	probe.release();

	EXPECT_EQ(firstCount, 2);
	EXPECT_EQ(secondCount, 0);
	EXPECT_FALSE(second.isValid());
}

TEST(LifeCycleTest, ReentrantTransitionsRemainConsistent)
{
	LifeCycleProbe probe;
	std::vector<std::string> calls;
	auto release = probe.subscribeToRelease([&]() {
		calls.push_back("release");
		probe.acquire();
	});
	auto acquisition = probe.subscribeToAcquisition([&]() {
		calls.push_back("acquisition");
	});

	probe.release();
	EXPECT_EQ(calls, (std::vector<std::string>{"release", "acquisition"}));
	EXPECT_TRUE(probe.isAcquired());

	probe.finalizeLifeCycle();
	EXPECT_EQ(calls, (std::vector<std::string>{"release", "acquisition", "release"}));
	EXPECT_TRUE(probe.isReleased());
	probe.acquire();
	EXPECT_TRUE(probe.isReleased());
}

TEST(LifeCycleTest, CopyDoesNotCopyStateOrSubscriptions)
{
	LifeCycleProbe source;
	source.release();
	auto subscribed = source.subscribeToAcquisition([]() {
	});

	LifeCycleProbe copy(source);
	EXPECT_TRUE(copy.isAcquired());
	EXPECT_TRUE(source.isReleased());

	auto copied = copy.subscribeToAcquisition([]() {
	});
	source = copy;
	EXPECT_TRUE(source.isReleased());
	EXPECT_TRUE(subscribed.isValid());
	EXPECT_TRUE(copied.isValid());
}
