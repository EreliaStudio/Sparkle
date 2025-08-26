#include "structure/math/spk_polygon_tester.hpp"

TEST_F(PolygonTest, ContainsInside)
{
	spk::Polygon outer = spk::Polygon::makeSquare({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

	spk::Polygon inner = spk::Polygon::makeSquare({0.2f, 0.2f, 0.0f}, {0.8f, 0.2f, 0.0f}, {0.8f, 0.8f, 0.0f}, {0.2f, 0.8f, 0.0f});

	EXPECT_TRUE(outer.contains(inner));
}

TEST_F(PolygonTest, ContainsSharedEdge)
{
	spk::Polygon outer = spk::Polygon::makeSquare({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

	spk::Polygon same = outer;

	EXPECT_TRUE(outer.contains(same));
}

TEST_F(PolygonTest, ContainsOutside)
{
	spk::Polygon outer = spk::Polygon::makeSquare({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

	spk::Polygon outside = spk::Polygon::makeSquare({-0.1f, 0.2f, 0.0f}, {1.1f, 0.2f, 0.0f}, {1.1f, 0.8f, 0.0f}, {-0.1f, 0.8f, 0.0f});

	EXPECT_FALSE(outer.contains(outside));
}

TEST_F(PolygonTest, ContainPointInside)
{
	spk::Polygon poly = spk::Polygon::makeSquare({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

	spk::Vector3 point(0.5f, 0.5f, 0.0f);

	EXPECT_TRUE(poly.contains(point));
}

TEST_F(PolygonTest, ContainPointEdge)
{
	spk::Polygon poly = spk::Polygon::makeSquare({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

	spk::Vector3 edgePoint(0.0f, 0.5f, 0.0f);

	EXPECT_TRUE(poly.contains(edgePoint));
}

TEST_F(PolygonTest, ContainPointOutside)
{
	spk::Polygon poly = spk::Polygon::makeSquare({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

	spk::Vector3 outsidePoint(-0.1f, 0.5f, 0.0f);

	EXPECT_FALSE(poly.contains(outsidePoint));
}

TEST_F(PolygonTest, ContainPointOffPlane)
{
	spk::Polygon poly = spk::Polygon::makeSquare({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

	spk::Vector3 above(0.5f, 0.5f, 0.1f);

	EXPECT_FALSE(poly.contains(above));
}
