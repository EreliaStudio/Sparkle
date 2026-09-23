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

	struct ThrowingCopyAssignableElement
	{
		int value = 0;
		bool throwOnCopy = false;

		ThrowingCopyAssignableElement() = default;
		ThrowingCopyAssignableElement(const ThrowingCopyAssignableElement &) = delete;
		ThrowingCopyAssignableElement(ThrowingCopyAssignableElement &&) = delete;

		ThrowingCopyAssignableElement &operator=(const ThrowingCopyAssignableElement &other)
		{
			value = other.value;

			if (other.throwOnCopy)
			{
				throw std::runtime_error("copy failed");
			}

			throwOnCopy = other.throwOnCopy;
			return *this;
		}

		ThrowingCopyAssignableElement &operator=(ThrowingCopyAssignableElement &&) = delete;
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

static_assert(std::is_copy_constructible_v<IntPool::Lease>);
static_assert(std::is_move_constructible_v<IntPool::Lease>);
static_assert(std::is_copy_assignable_v<IntPool::Lease>);
static_assert(std::is_move_assignable_v<IntPool::Lease>);

using NonMovablePool = spk::Pool<NonMovableElement>;

static_assert(!std::is_copy_constructible_v<NonMovablePool::Lease>);
static_assert(!std::is_copy_assignable_v<NonMovablePool::Lease>);

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

TEST(PoolTest, LeaseCopyConstructsIndependentElementFromSamePool)
{
	int obtainCount = 0;

	IntPool pool(
		[]() {
			return new int();
		},
		[&](int &value) {
			++obtainCount;
			value = -1;
		});

	auto first = pool.obtain();
	*first = 73;

	auto second = first;

	EXPECT_NE(second.get(), first.get());
	EXPECT_EQ(*first, 73);
	EXPECT_EQ(*second, 73);
	EXPECT_EQ(obtainCount, 2);

	*second = 91;

	EXPECT_EQ(*first, 73);
	EXPECT_EQ(*second, 91);
}

TEST(PoolTest, LeaseCopyAssignmentRecyclesPreviousDestinationAfterSuccessfulCopy)
{
	IntPool pool;

	auto source = pool.obtain();
	auto destination = pool.obtain();

	*source = 12;
	*destination = 48;

	int *previousDestination = destination.get();

	destination = source;

	EXPECT_NE(destination.get(), source.get());
	EXPECT_EQ(*source, 12);
	EXPECT_EQ(*destination, 12);
	ASSERT_EQ(pool.available(), 1u);

	auto recycled = pool.obtain();
	EXPECT_EQ(recycled.get(), previousDestination);
	EXPECT_EQ(*recycled, 48);
}

TEST(PoolTest, LeaseCopyAfterPoolDestructionThrows)
{
	IntPool::Lease source;

	{
		IntPool pool;
		source = pool.obtain();
		*source = 37;
	}

	EXPECT_THROW(
		{
			IntPool::Lease copy(source);
		},
		spk::Exception);

	EXPECT_EQ(*source, 37);
}

TEST(PoolTest, ThrowingLeaseCopyDiscardsNewElementAndLeavesDestinationUntouched)
{
	using Element = ThrowingCopyAssignableElement;
	using ElementPool = spk::Pool<Element>;

	ElementPool pool;

	auto source = pool.obtain();
	source->value = 27;
	source->throwOnCopy = true;

	auto destination = pool.obtain();
	destination->value = 84;

	Element *destinationAddress = destination.get();

	EXPECT_THROW(destination = source, std::runtime_error);

	EXPECT_EQ(destination.get(), destinationAddress);
	EXPECT_EQ(destination->value, 84);
	EXPECT_EQ(pool.available(), 0u);
}
