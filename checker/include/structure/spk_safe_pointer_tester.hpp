#pragma once

#include <gtest/gtest.h>
#include "structure/spk_safe_pointer.hpp" // Adjust the include path as necessary

namespace spk
{
	class TestObject
	{
	public:
		TestObject(int value) : _value(value) {}

		int getValue() const
		{
			return _value;
		}

		void setValue(int value)
		{
			_value = value;
		}

	private:
		int _value;
	};
}

class SafePointerTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		obj = new spk::TestObject(42);
		ptr = spk::SafePointer<spk::TestObject>(obj);
	}

	void TearDown() override
	{
		delete obj;
	}

	spk::TestObject* obj;
	spk::SafePointer<spk::TestObject> ptr;
};