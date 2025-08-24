#include "structure/math/spk_polygon_tester.hpp"
#include <vector>

TEST_F(PolygonTest, ContainsInside)
{
	spk::Polygon outer;
	outer.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

	spk::Polygon inner;
	inner.points = {{0.2f, 0.2f, 0.0f}, {0.8f, 0.2f, 0.0f}, {0.8f, 0.8f, 0.0f}, {0.2f, 0.8f, 0.0f}};

	EXPECT_TRUE(outer.contains(inner));
}

TEST_F(PolygonTest, ContainsSharedEdge)
{
	spk::Polygon outer;
	outer.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

	spk::Polygon same;
	same.points = outer.points;

	EXPECT_TRUE(outer.contains(same));
}

TEST_F(PolygonTest, ContainsOutside)
{
	spk::Polygon outer;
	outer.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

	spk::Polygon outside;
	outside.points = {{-0.1f, 0.2f, 0.0f}, {1.1f, 0.2f, 0.0f}, {1.1f, 0.8f, 0.0f}, {-0.1f, 0.8f, 0.0f}};

	EXPECT_FALSE(outer.contains(outside));
}

TEST_F(PolygonTest, ContainPointInside)
{
	spk::Polygon poly;
	poly.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

	spk::Vector3 point(0.5f, 0.5f, 0.0f);

	EXPECT_TRUE(poly.contains(point));
}

TEST_F(PolygonTest, ContainPointEdge)
{
	spk::Polygon poly;
	poly.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

	spk::Vector3 edgePoint(0.0f, 0.5f, 0.0f);

	EXPECT_TRUE(poly.contains(edgePoint));
}

TEST_F(PolygonTest, ContainPointOutside)
{
	spk::Polygon poly;
	poly.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

	spk::Vector3 outsidePoint(-0.1f, 0.5f, 0.0f);

	EXPECT_FALSE(poly.contains(outsidePoint));
}

TEST_F(PolygonTest, ContainPointOffPlane)
{
	spk::Polygon poly;
	poly.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

	spk::Vector3 above(0.5f, 0.5f, 0.1f);

	EXPECT_FALSE(poly.contains(above));
}

TEST_F(PolygonTest, RewindOrder)
{
	spk::Polygon poly;
	poly.points = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};

	poly.rewind(spk::Polygon::Winding::CounterClockwise);

	float area = 0.0f;
	for (size_t i = 0; i < poly.points.size(); ++i)
	{
		const spk::Vector3 &a = poly.points[i];
		const spk::Vector3 &b = poly.points[(i + 1) % poly.points.size()];
		area += (a.x * b.y - b.x * a.y);
	}
	EXPECT_TRUE(area > 0.0f);

	poly.rewind(spk::Polygon::Winding::Clockwise);
	area = 0.0f;
	for (size_t i = 0; i < poly.points.size(); ++i)
	{
		const spk::Vector3 &a = poly.points[i];
		const spk::Vector3 &b = poly.points[(i + 1) % poly.points.size()];
		area += (a.x * b.y - b.x * a.y);
	}
	EXPECT_TRUE(area < 0.0f);
}

TEST_F(PolygonTest, ConvexityCheck)
{
	spk::Polygon square;
	square.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
	EXPECT_TRUE(square.isConvex());

	spk::Polygon concave;
	concave.points = {{0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 2.0f, 0.0f}, {0.0f, 2.0f, 0.0f}};
	EXPECT_FALSE(concave.isConvex());
}

TEST_F(PolygonTest, MergeAdjacent)
{
	spk::Polygon left;
	left.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};

	spk::Polygon right;
	right.points = {{1.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};

	EXPECT_TRUE(left.isAdjacent(right));
	EXPECT_TRUE(left.isMergable(right));

	left.merge(right);

	EXPECT_EQ(left.points.size(), 4u);
	EXPECT_TRUE(left.contains(spk::Vector3(1.5f, 0.5f, 0.0f)));
}

TEST_F(PolygonTest, TriangulizeSquare)
{
	spk::Polygon square;
	square.points = {{0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 0.0f}, {0.0f, 2.0f, 0.0f}};
	std::vector<spk::Polygon> tris = square.triangulize();
	EXPECT_EQ(tris.size(), 2u);
	for (const spk::Polygon &t : tris)
	{
		EXPECT_EQ(t.points.size(), 3u);
	}
}

TEST_F(PolygonTest, SplitConcave)
{
	spk::Polygon concave;
	concave.points = {{0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 2.0f, 0.0f}, {0.0f, 2.0f, 0.0f}};
	std::vector<spk::Polygon> parts = concave.split();
	EXPECT_EQ(parts.size(), 4u);
	for (const spk::Polygon &t : parts)
	{
		EXPECT_EQ(t.points.size(), 3u);
	}
}
