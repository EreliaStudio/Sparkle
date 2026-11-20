#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/entity.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"

TEST(EntityLifeCycleTest, EntityNotifiesReleaseBeforeDestruction)
{
	std::vector<std::string> calls;
	auto entity = std::make_unique<spk::Entity>("headless");
	auto release = entity->subscribeToRelease([&]() {
		calls.push_back("release");
	});
	auto destruction = entity->subscribeToDestruction([&]() {
		calls.push_back("destruction");
	});

	EXPECT_TRUE(entity->isAcquired());
	entity.reset();

	EXPECT_EQ(calls, (std::vector<std::string>{"release", "destruction"}));
	EXPECT_FALSE(destruction.isValid());
}

TEST(EntityLifeCycleTest, Entity3DNotifiesWhileItsComponentsAreStillAlive)
{
	std::vector<std::string> calls;
	auto entity = std::make_unique<spk::Entity3D>("3d");
	spk::Entity3D *observed = entity.get();
	auto release = entity->subscribeToRelease([&]() {
		EXPECT_NE(observed->getComponent<spk::Transform3D>(), nullptr);
		calls.push_back("release");
	});
	auto destruction = entity->subscribeToDestruction([&]() {
		EXPECT_NE(observed->getComponent<spk::Transform3D>(), nullptr);
		calls.push_back("destruction");
	});

	entity.reset();
	EXPECT_EQ(calls, (std::vector<std::string>{"release", "destruction"}));
}

TEST(EntityLifeCycleTest, EntityDestructorSuppressesCallbackExceptions)
{
	auto entity = std::make_unique<spk::Entity>("throwing");
	int destructionCount = 0;
	auto release = entity->subscribeToRelease([]() {
		throw std::runtime_error("release failed");
	});
	auto destruction = entity->subscribeToDestruction([&]() {
		++destructionCount;
	});

	EXPECT_NO_THROW(entity.reset());
	EXPECT_EQ(destructionCount, 1);
}
