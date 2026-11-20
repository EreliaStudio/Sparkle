#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "container/pool.hpp"
#include "design_pattern/life_cycle.hpp"

namespace
{
	struct PooledLifeCycle : spk::LifeCycle
	{
		int value = 0;
	};
}

TEST(PoolLifeCycleTest, FirstObtainReusesElementAndClearsOnlyReleasedObjects)
{
	std::vector<std::string> calls;
	std::vector<spk::LifeCycle::DestructionContract> contracts;
	spk::Pool<PooledLifeCycle> pool([&]() {
		auto *element = new PooledLifeCycle();
		contracts.push_back(element->subscribeToAcquisition([&]() {
			calls.push_back("acquire");
		}));
		contracts.push_back(element->subscribeToRelease([&]() {
			calls.push_back("release");
		}));
		contracts.push_back(element->subscribeToDestruction([&]() {
			calls.push_back("destroy");
		}));
		return element;
	});

	PooledLifeCycle *address = nullptr;
	{
		auto first = pool.obtain();
		address = first.get();
		EXPECT_TRUE(first->isAcquired());
		EXPECT_EQ(calls, (std::vector<std::string>{"release", "acquire"}));
	}

	EXPECT_EQ(calls, (std::vector<std::string>{"release", "acquire", "release"}));
	ASSERT_EQ(pool.available(), 1u);
	{
		auto second = pool.obtain();
		EXPECT_EQ(second.get(), address);
		EXPECT_TRUE(second->isAcquired());
	}
	EXPECT_EQ(pool.available(), 1u);
	pool.clear();

	EXPECT_EQ(
		calls,
		(std::vector<std::string>{"release", "acquire", "release", "acquire", "release", "destroy"}));
}

TEST(PoolLifeCycleTest, LeaseCanOutlivePoolAndIsReleasedBeforeDeletion)
{
	std::vector<std::string> calls;
	std::vector<spk::LifeCycle::DestructionContract> contracts;
	spk::Pool<PooledLifeCycle>::Lease lease;
	{
		spk::Pool<PooledLifeCycle> pool([&]() {
			auto *element = new PooledLifeCycle();
			contracts.push_back(element->subscribeToRelease([&]() {
				calls.push_back("release");
			}));
			contracts.push_back(element->subscribeToDestruction([&]() {
				calls.push_back("destroy");
			}));
			return element;
		});
		lease = pool.obtain();
		EXPECT_EQ(calls, (std::vector<std::string>{"release"}));
	}
	EXPECT_TRUE(lease);
	EXPECT_TRUE(lease->isAcquired());
	lease = {};

	EXPECT_EQ(calls, (std::vector<std::string>{"release", "release", "destroy"}));
}

TEST(PoolLifeCycleTest, CopiedLeaseAcquiresItsOwnElementAndCopiesValue)
{
	int acquisitionCount = 0;
	int releaseCount = 0;
	std::vector<spk::LifeCycle::AcquisitionContract> contracts;
	spk::Pool<PooledLifeCycle> pool([&]() {
		auto *element = new PooledLifeCycle();
		contracts.push_back(element->subscribeToAcquisition([&]() {
			++acquisitionCount;
		}));
		contracts.push_back(element->subscribeToRelease([&]() {
			++releaseCount;
		}));
		return element;
	});

	{
		auto first = pool.obtain();
		first->value = 42;
		auto copy = first;
		EXPECT_NE(copy.get(), first.get());
		EXPECT_EQ(copy->value, 42);
		EXPECT_TRUE(copy->isAcquired());
		EXPECT_EQ(acquisitionCount, 2);

		auto target = pool.obtain();
		target->value = 99;
		target = first;
		EXPECT_EQ(target->value, 42);
		EXPECT_TRUE(target->isAcquired());
		EXPECT_EQ(acquisitionCount, 4);
	}

	EXPECT_EQ(pool.available(), 3u);
	EXPECT_EQ(releaseCount, 7);
}

TEST(PoolLifeCycleTest, ThrowingInitializationReleasesAndDestroysTheAcquiredElement)
{
	std::vector<std::string> calls;
	std::vector<spk::LifeCycle::DestructionContract> contracts;
	spk::Pool<PooledLifeCycle> pool([&]() {
		auto *element = new PooledLifeCycle();
		contracts.push_back(element->subscribeToAcquisition([&]() {
			calls.push_back("acquire");
		}));
		contracts.push_back(element->subscribeToRelease([&]() {
			calls.push_back("release");
		}));
		contracts.push_back(element->subscribeToDestruction([&]() {
			calls.push_back("destroy");
		}));
		return element;
	});

	EXPECT_THROW(
		(void)pool.obtain([](PooledLifeCycle &) {
			throw std::runtime_error("init failed");
		}),
		spk::Exception);
	EXPECT_EQ(pool.available(), 0u);
	EXPECT_EQ(calls, (std::vector<std::string>{"release", "acquire", "release", "destroy"}));
}

TEST(PoolLifeCycleTest, ReentrantFactoryReleaseCanObtainAnotherElement)
{
	int created = 0;
	int nested = 0;
	std::vector<spk::LifeCycle::ReleaseContract> contracts;
	spk::Pool<PooledLifeCycle> pool([&]() {
		auto *element = new PooledLifeCycle();
		++created;
		if (created == 1)
		{
			contracts.push_back(element->subscribeToRelease([&]() {
				if (nested == 0)
				{
					++nested;
					auto inner = pool.obtain();
					EXPECT_TRUE(inner->isAcquired());
				}
			}));
		}
		return element;
	});

	auto outer = pool.obtain();
	EXPECT_EQ(nested, 1);
	EXPECT_EQ(created, 2);
	EXPECT_TRUE(outer->isAcquired());
	EXPECT_EQ(pool.available(), 1u);
}

TEST(PoolLifeCycleTest, SubscriberExceptionDoesNotPreventLeaseRecycling)
{
	int releaseCount = 0;
	std::vector<spk::LifeCycle::ReleaseContract> contracts;
	spk::Pool<PooledLifeCycle> pool([&]() {
		auto *element = new PooledLifeCycle();
		contracts.push_back(element->subscribeToRelease([&]() {
			++releaseCount;
			if (releaseCount > 1)
			{
				throw std::runtime_error("return failure");
			}
		}));
		return element;
	});

	{
		auto first = pool.obtain();
	}
	EXPECT_EQ(pool.available(), 1u);
	EXPECT_EQ(releaseCount, 2);
	auto second = pool.obtain();
	EXPECT_TRUE(second->isAcquired());
}

TEST(PoolLifeCycleTest, MoveAssignmentRecyclesPreviousLeaseExactlyOnce)
{
	std::vector<std::string> calls;
	std::vector<spk::LifeCycle::ReleaseContract> contracts;
	spk::Pool<PooledLifeCycle> pool([&]() {
		auto *element = new PooledLifeCycle();
		contracts.push_back(element->subscribeToRelease([&]() {
			calls.push_back("release");
		}));
		return element;
	});

	auto first = pool.obtain();
	auto second = pool.obtain();
	EXPECT_EQ(calls.size(), 2u);
	second = std::move(first);
	EXPECT_FALSE(first);
	EXPECT_EQ(calls.size(), 3u);
	EXPECT_EQ(pool.available(), 1u);
}
