#pragma once

#include <gtest/gtest.h>

#include "structure/design_pattern/spk_singleton.hpp"

namespace
{
	class TestClass
	{
	public:
		TestClass(int value) : value(value) {}

		int getValue() const { return value; }

	private:
		int value;
	};

	using TestSingleton = spk::Singleton<TestClass>;
}

class SingletonTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Ensure the singleton is released before each test
		::TestSingleton::release();
	}

	void TearDown() override
	{
		// Ensure the singleton is released after each test
		::TestSingleton::release();
	}
};