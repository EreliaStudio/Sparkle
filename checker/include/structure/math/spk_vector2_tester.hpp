#pragma once

#include <gtest/gtest.h>
#include "structure/math/spk_vector2.hpp"

class Vector2Test : public ::testing::Test
{
protected:

	virtual void SetUp()
	{

	}
};

namespace spk
{
	template<typename TType>
	void PrintTo(const IVector2<TType>& vec, std::ostream* os)
	{
		*os << vec;
	}
}