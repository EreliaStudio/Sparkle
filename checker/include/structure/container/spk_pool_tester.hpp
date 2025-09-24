#pragma once

#include "gtest/gtest.h"

#include "structure/container/spk_pool.hpp"

class PoolTest : public ::testing::Test
{
protected:
	struct TestObject
	{
		int value;
		TestObject() :
			value(0)
		{
		}
		explicit TestObject(int initialValue) :
			value(initialValue)
		{
		}
	};

	using TestPool = spk::Pool<TestObject>;

	TestPool pool;
};