#include <gtest/gtest.h>

#include "container/pool.hpp"

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
	struct NonMovableElement
	{
		int value;

		explicit NonMovableElement(int value) :
			value(value)
		{
		}

		NonMovableElement(const NonMovableElement &) = delete;
		NonMovableElement(NonMovableElement &&) = delete;

		NonMovableElement &operator=(const NonMovableElement &) = delete;
		NonMovableElement &operator=(NonMovableElement &&) = delete;
	};

	struct TrackedElement
	{
		std::atomic<int> *destructionCount;

		explicit TrackedElement(std::atomic<int> &destructionCount) :
			destructionCount(&destructionCount)
		{
		}

		TrackedElement(const TrackedElement &) = delete;
		TrackedElement(TrackedElement &&) = delete;

		TrackedElement &operator=(const TrackedElement &) = delete;
		TrackedElement &operator=(TrackedElement &&) = delete;

		~TrackedElement()
		{
			destructionCount->fetch_add(1, std::memory_order_relaxed);
		}
	};
}

using IntPool = spk::Pool<int>;

static_assert(!std::is_copy_constructible_v<IntPool>);
static_assert(!std::is_move_constructible_v<IntPool>);
static_assert(!std::is_copy_assignable_v<IntPool>);
static_assert(!std::is_move_assignable_v<IntPool>);

static_assert(!std::is_copy_constructible_v<IntPool::Lease>);
static_assert(std::is_move_constructible_v<IntPool::Lease>);
static_assert(!std::is_copy_assignable_v<IntPool::Lease>);
static_assert(std::is_move_assignable_v<IntPool::Lease>);

TEST(PoolTest, DefaultFactoryCreatesAndRecyclesElement)
{
	IntPool pool;
	int *firstAddress = nullptr;

	{
		auto first = pool.obtain();
		ASSERT_TRUE(first);
		EXPECT_EQ(*first, 0);

		firstAddress = first.get();
		*first = 42;
	}

	ASSERT_EQ(pool.available(), 1u);

	{
		auto second = pool.obtain();
		EXPECT_EQ(second.get(), firstAddress);
		EXPECT_EQ(*second, 42);
	}

	EXPECT_EQ(pool.available(), 1u);
}

TEST(PoolTest, OnObtainRunsForNewAndRecycledElements)
{
	int obtainCount = 0;

	IntPool pool([&](int &value) {
		++obtainCount;
		value = 7;
	});

	{
		auto first = pool.obtain();
		EXPECT_EQ(*first, 7);
		*first = 99;
	}

	{
		auto second = pool.obtain();
		EXPECT_EQ(*second, 7);
	}

	EXPECT_EQ(obtainCount, 2);
}

TEST(PoolTest, OnObtainCanClearVectorWithoutReleasingCapacity)
{
	using Buffer = std::vector<int>;

	spk::Pool<Buffer> pool([](Buffer &buffer) {
		buffer.clear();
	});

	Buffer *firstAddress = nullptr;
	std::size_t retainedCapacity = 0;

	{
		auto first = pool.obtain();
		firstAddress = first.get();

		first->reserve(128);
		first->resize(64, 12);
		retainedCapacity = first->capacity();

		ASSERT_GE(retainedCapacity, 128u);
		ASSERT_EQ(first->size(), 64u);
	}

	{
		auto second = pool.obtain();

		EXPECT_EQ(second.get(), firstAddress);
		EXPECT_TRUE(second->empty());
		EXPECT_EQ(second->capacity(), retainedCapacity);
	}
}

TEST(PoolTest, CustomFactorySupportsNonDefaultConstructibleNonMovableElements)
{
	spk::Pool<NonMovableElement> pool([]() {
		return new NonMovableElement(73);
	});

	NonMovableElement *firstAddress = nullptr;

	{
		auto first = pool.obtain();
		firstAddress = first.get();
		EXPECT_EQ(first->value, 73);
		first->value = 91;
	}

	{
		auto second = pool.obtain();
		EXPECT_EQ(second.get(), firstAddress);
		EXPECT_EQ(second->value, 91);
	}
}

TEST(PoolTest, ThrowingOnObtainDiscardsElementAndRethrows)
{
	std::atomic<int> destructionCount = 0;

	spk::Pool<TrackedElement> pool(
		[&]() {
			return new TrackedElement(destructionCount);
		},
		[](TrackedElement &) {
			throw std::runtime_error("prepare failed");
		});

	EXPECT_THROW((void)pool.obtain(), std::runtime_error);
	EXPECT_EQ(pool.available(), 0u);
	EXPECT_EQ(destructionCount.load(std::memory_order_relaxed), 1);
}

TEST(PoolTest, NullFactoryResultIsRejected)
{
	spk::Pool<int> pool([]() -> int * {
		return nullptr;
	});

	EXPECT_THROW((void)pool.obtain(), spk::Exception);
	EXPECT_EQ(pool.available(), 0u);
}

TEST(PoolTest, LeaseCanOutliveOriginatingPool)
{
	std::atomic<int> destructionCount = 0;
	spk::Pool<TrackedElement>::Lease lease;

	{
		spk::Pool<TrackedElement> pool([&]() {
			return new TrackedElement(destructionCount);
		});

		lease = pool.obtain();
		EXPECT_EQ(destructionCount.load(std::memory_order_relaxed), 0);
	}

	EXPECT_EQ(destructionCount.load(std::memory_order_relaxed), 0);

	lease = {};

	EXPECT_EQ(destructionCount.load(std::memory_order_relaxed), 1);
}

TEST(PoolTest, LeaseMoveAssignmentRecyclesPreviouslyHeldElement)
{
	IntPool pool;

	auto first = pool.obtain();
	auto second = pool.obtain();

	int *firstAddress = first.get();
	int *secondAddress = second.get();

	ASSERT_NE(firstAddress, secondAddress);
	EXPECT_EQ(pool.available(), 0u);

	second = std::move(first);

	EXPECT_FALSE(first);
	EXPECT_EQ(second.get(), firstAddress);
	EXPECT_EQ(pool.available(), 1u);

	auto recycled = pool.obtain();
	EXPECT_EQ(recycled.get(), secondAddress);
}

TEST(PoolTest, ClearDestroysAvailableElementsWithoutTouchingOutstandingLeases)
{
	std::atomic<int> destructionCount = 0;

	spk::Pool<TrackedElement> pool([&]() {
		return new TrackedElement(destructionCount);
	});

	auto outstanding = pool.obtain();

	{
		auto returned = pool.obtain();
	}

	ASSERT_EQ(pool.available(), 1u);

	pool.clear();

	EXPECT_EQ(pool.available(), 0u);
	EXPECT_EQ(destructionCount.load(std::memory_order_relaxed), 1);
	ASSERT_TRUE(outstanding);

	outstanding = {};

	EXPECT_EQ(destructionCount.load(std::memory_order_relaxed), 2);
}

TEST(PoolTest, SetFactoryAffectsOnlyFutureAllocations)
{
	int firstFactoryCount = 0;
	int secondFactoryCount = 0;

	spk::Pool<int> pool([&]() {
		++firstFactoryCount;
		return new int(11);
	});

	int *recycledAddress = nullptr;

	{
		auto first = pool.obtain();
		recycledAddress = first.get();
		EXPECT_EQ(*first, 11);
	}

	pool.setFactory([&]() {
		++secondFactoryCount;
		return new int(22);
	});

	{
		auto recycled = pool.obtain();
		EXPECT_EQ(recycled.get(), recycledAddress);
		EXPECT_EQ(*recycled, 11);
	}

	auto recycled = pool.obtain();
	auto fresh = pool.obtain();

	EXPECT_EQ(*fresh, 22);
	EXPECT_EQ(firstFactoryCount, 1);
	EXPECT_EQ(secondFactoryCount, 1);
}

TEST(PoolTest, SetFactoryRejectsEmptyFactory)
{
	IntPool pool;

	EXPECT_THROW(pool.setFactory({}), spk::Exception);
}

TEST(PoolTest, SetOnObtainAffectsSubsequentObtainsAndCanBeDisabled)
{
	IntPool pool;

	pool.setOnObtain([](int &value) {
		value = 41;
	});

	{
		auto value = pool.obtain();
		EXPECT_EQ(*value, 41);
		*value = 9;
	}

	pool.setOnObtain({});

	{
		auto value = pool.obtain();
		EXPECT_EQ(*value, 9);
	}
}
