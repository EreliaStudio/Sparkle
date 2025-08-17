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

TEST_F(PolygonTest, ContainPointInside)
{
        spk::Polygon poly;
        poly.points = { {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };

        spk::Vector3 point(0.5f, 0.5f, 0.0f);

        EXPECT_TRUE(poly.containPoint(point));
}

TEST_F(PolygonTest, ContainPointEdge)
{
        spk::Polygon poly;
        poly.points = { {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };

        spk::Vector3 edgePoint(0.0f, 0.5f, 0.0f);

        EXPECT_TRUE(poly.containPoint(edgePoint));
}

TEST_F(PolygonTest, ContainPointOutside)
{
        spk::Polygon poly;
        poly.points = { {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };

        spk::Vector3 outsidePoint(-0.1f, 0.5f, 0.0f);

        EXPECT_FALSE(poly.containPoint(outsidePoint));
}

TEST_F(PolygonTest, ContainPointOffPlane)
{
        spk::Polygon poly;
        poly.points = { {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} };

        spk::Vector3 above(0.5f, 0.5f, 0.1f);

        EXPECT_FALSE(poly.containPoint(above));
}

