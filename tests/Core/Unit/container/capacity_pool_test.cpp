#include <gtest/gtest.h>

#include "container/capacity_pool.hpp"

#include <cstddef>
#include <thread>
#include <vector>

namespace
{
	struct Element
	{
		std::size_t capacity;
	};

	using ElementPool = spk::CapacityPool<Element>;

	Element *createElement(std::size_t capacity)
	{
		return new Element{capacity};
	}

	std::size_t capacityOf(const Element &element)
	{
		return element.capacity;
	}
}

TEST(CapacityPoolTest, FixedCapacityRetainsOriginalCategory)
{
	ElementPool pool(createElement);
	auto lease = pool.obtain(3);
	auto *address = lease.get();
	lease->capacity = 8;
	lease = {};
	EXPECT_EQ(pool.obtain(4).get(), address);
}

TEST(CapacityPoolTest, RecyclesGrownElementIntoNewCategory)
{
	ElementPool pool(createElement, capacityOf);
	auto lease = pool.obtain(4);
	auto *address = lease.get();
	lease->capacity = 16;
	lease = {};
	auto smaller = pool.obtain(4);
	EXPECT_NE(smaller.get(), address);
	EXPECT_EQ(pool.obtain(16).get(), address);
}

TEST(CapacityPoolTest, RoundsActualCapacityDownWhenRecycling)
{
	ElementPool pool(createElement, capacityOf);
	auto lease = pool.obtain(16);
	auto *address = lease.get();
	lease->capacity = 6;
	lease = {};
	auto larger = pool.obtain(8);
	EXPECT_GE(larger->capacity, 8u);
	EXPECT_NE(larger.get(), address);
	EXPECT_EQ(pool.obtain(4).get(), address);
}

TEST(CapacityPoolTest, CopiedLeaseHasIndependentElementAndRecyclesByCapacity)
{
	ElementPool pool(createElement, capacityOf);
	auto original = pool.obtain(4);
	original->capacity = 16;
	auto copy = original;
	auto *address = copy.get();
	EXPECT_NE(original.get(), address);
	copy = {};
	EXPECT_EQ(pool.obtain(16).get(), address);
}

TEST(CapacityPoolTest, LeaseCanOutliveDynamicPool)
{
	ElementPool::Lease lease;
	{
		ElementPool pool(createElement, capacityOf);
		lease = pool.obtain(4);
		lease->capacity = 16;
	}
	EXPECT_EQ(lease->capacity, 16u);
	lease = {};
	EXPECT_FALSE(lease);
}

TEST(CapacityPoolTest, ConcurrentReclassification)
{
	ElementPool pool(createElement, capacityOf);
	std::vector<std::jthread> threads;
	for (int index = 0; index < 8; ++index)
	{
		threads.emplace_back([&pool]() {
			for (int iteration = 0; iteration < 100; ++iteration)
			{
				auto lease = pool.obtain(4);
				EXPECT_GE(lease->capacity, 4u);
				lease->capacity = 8;
				auto larger = pool.obtain(8);
				EXPECT_GE(larger->capacity, 8u);
				larger->capacity = 4;
			}
		});
	}
}
