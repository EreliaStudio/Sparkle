#include "structure/container/spk_pool_tester.hpp"

TEST_F(PoolTest, AllocateAndObtain)
{
	pool.allocate();
	ASSERT_EQ(pool.size(), 1) << "Pool size should be 1 after allocation";

	auto obj = pool.obtain();
	ASSERT_EQ(pool.size(), 0) << "Pool size should be 0 after obtaining an object";

	ASSERT_NE(obj, nullptr) << "Obtained object should not be null";
	ASSERT_EQ(obj->value, 0) << "Obtained object value should be 0";
}

TEST_F(PoolTest, ResizePool)
{
	pool.resize(5);
	ASSERT_EQ(pool.size(), 5) << "Pool size should be 5 after resizing";

	{
		TestPool::Object object[5];

		for (int i = 0; i < 5; ++i)
		{
			object[i] = std::move(pool.obtain());

			ASSERT_NE(object[i], nullptr) << "Obtained object should not be null";
			ASSERT_EQ(pool.size(), 5 - (i + 1)) << "Pool size should be 5 after obtaining all objects";
		}

		ASSERT_EQ(pool.size(), 0) << "Pool size should be 0 after obtaining all objects";
	}
	ASSERT_EQ(pool.size(), 5) << "Pool size should be 5 after releasing all objects";
}

TEST_F(PoolTest, MultipleObtainsAndReturns)
{
	pool.resize(3);
	ASSERT_EQ(pool.size(), 3) << "Pool size should be 3 after resizing";

	{
		auto obj1 = pool.obtain();
		auto obj2 = pool.obtain();

		ASSERT_EQ(pool.size(), 1) << "Pool size should be 1 after obtaining two objects";

		obj1.reset();
		ASSERT_EQ(pool.size(), 2) << "Pool size should be 2 after returning one object";

		obj2.reset();
		ASSERT_EQ(pool.size(), 3) << "Pool size should be 3 after returning both objects";
	}
}

TEST_F(PoolTest, CleanerFunctionality)
{
	pool.resize(2);

	auto obj1 = pool.obtain();
	obj1->value = 123;
	obj1.reset();

	auto obj2 = pool.obtain();
	ASSERT_EQ(obj2->value, 0) << "Object value should be 0 after cleaner is applied";
}

TEST_F(PoolTest, SetMaximumAllocationSizeTrimsStoredObjects)
{
	pool.resize(5);
	ASSERT_EQ(pool.size(), 5U) << "Initial resize should create five cached objects";

	pool.setMaximumAllocationSize(2);
	EXPECT_EQ(pool.size(), 2U) << "Resizing the maximum should trim the stored objects";
}

TEST_F(PoolTest, ReleaseClearsAllObjects)
{
	pool.resize(3);
	ASSERT_EQ(pool.size(), 3U) << "Pool should contain three objects before release";

	pool.release();
	EXPECT_EQ(pool.size(), 0U) << "Release should remove every cached object";
}

TEST_F(PoolTest, ObtainConstructsWithForwardedArguments)
{
	auto object = pool.obtain(42);
	ASSERT_NE(object, nullptr) << "Obtained object should not be null when created with arguments";
	EXPECT_EQ(object->value, 42) << "Obtained object should be constructed with the forwarded argument";
}

TEST_F(PoolTest, ReturnedObjectsObeyMaximumSizeLimit)
{
	pool.setMaximumAllocationSize(1);

	{
		auto first = pool.obtain();
		auto second = pool.obtain();
		ASSERT_NE(first, nullptr) << "First obtained object should be valid";
		ASSERT_NE(second, nullptr) << "Second obtained object should be valid";
	}

	EXPECT_EQ(pool.size(), 1U) << "After returning objects the cache should not exceed the maximum size";
}