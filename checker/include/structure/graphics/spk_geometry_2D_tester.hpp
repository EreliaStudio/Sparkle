#pragma once

#include <gtest/gtest.h>
#include "structure/graphics/spk_geometry_2D.hpp"

class Geometry2DTest : public ::testing::Test
{
protected:
	spk::Geometry2D geometry;
	spk::Geometry2D geometryInt;

	virtual void SetUp()
	{
		geometry = spk::Geometry2D();
		geometryInt = spk::Geometry2D();
	}
};