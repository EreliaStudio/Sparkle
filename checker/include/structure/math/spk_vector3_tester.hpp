#pragma once

#include <gtest/gtest.h>
#include "structure/math/spk_vector3.hpp"

class Vector3Test : public ::testing::Test
{
protected:

	virtual void SetUp()
	{

	}
};

namespace spk
{
	template<typename TType>
	void PrintTo(const IVector3<TType>& vec, std::ostream* os)
	{
		*os << vec;
	}
}