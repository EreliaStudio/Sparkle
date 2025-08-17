#include "structure/math/spk_polygon_tester.hpp"

TEST_F(PolygonTest, ContainsInside)
{
	spk::Polygon outer;
	outer.points = { {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };
	
	spk::Polygon inner;
	inner.points = { {0.2f, 0.2f, 0.0f}, {0.8f, 0.2f, 0.0f}, {0.8f, 0.8f, 0.0f}, {0.2f, 0.8f, 0.0f} };
	
	EXPECT_TRUE(outer.contains(inner));
}

TEST_F(PolygonTest, ContainsSharedEdge)
{
	spk::Polygon outer;
	outer.points = { {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };
	
	spk::Polygon same;
	same.points = outer.points;
	
	EXPECT_TRUE(outer.contains(same));
}

TEST_F(PolygonTest, ContainsOutside)
{
	spk::Polygon outer;
	outer.points = { {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };
	
	spk::Polygon outside;
	outside.points = { {-0.1f, 0.2f, 0.0f}, {1.1f, 0.2f, 0.0f}, {1.1f, 0.8f, 0.0f}, {-0.1f, 0.8f, 0.0f} };
	
	EXPECT_FALSE(outer.contains(outside));
}

