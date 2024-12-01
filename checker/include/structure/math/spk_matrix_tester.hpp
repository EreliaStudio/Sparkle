#pragma once

#include "gtest/gtest.h"
#include "structure/math/spk_matrix.hpp"

class MatrixTest : public ::testing::Test
{
protected:

	void SetUp() override
	{

	}
};


namespace spk
{
	template<size_t SizeX, size_t SizeY>
	void PrintTo(const IMatrix<SizeX, SizeY>& matrix, std::ostream* os)
	{
		*os << matrix;
	}
}