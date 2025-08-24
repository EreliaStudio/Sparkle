#include "structure/math/spk_polygon_tester.hpp"
#include <vector>

TEST_F(PolygonTest, MergeSquares)
{
	spk::Polygon left;
	left.addQuad({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

	spk::Polygon right;
	right.addQuad({1.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f});

	ASSERT_TRUE(left.canInsert(right));
	left.addPolygon(right);

	const auto &wire = left.pointsRef();
	EXPECT_EQ(wire.size(), 4u);
}

TEST_F(PolygonTest, OrientationAndRewind)
{
	spk::Polygon tri;
	tri.addTriangle({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

	const auto &pts = tri.pointsRef();
	ASSERT_EQ(pts.size(), 3u);
	auto rewired = tri.rewind();
	EXPECT_EQ(rewired, pts);
}

TEST_F(PolygonTest, TriangulizeSquare)
{
	spk::Polygon square;
	square.addQuad({0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 0.0f}, {0.0f, 2.0f, 0.0f});
	std::vector<spk::Polygon> tris = square.triangulize();
	EXPECT_EQ(tris.size(), 2u);
	for (const auto &t : tris)
	{
		const auto &wire = t.pointsRef();
		EXPECT_EQ(wire.size(), 3u);
	}
}
