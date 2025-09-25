#include "structure/math/spk_polygon_2d_tester.hpp"

#include <numeric>

TEST_F(Polygon2DTest, AreaPerimeterAndCentroid)
{
	spk::Polygon2D square =
		spk::Polygon2D::makeSquare(spk::Vector2(-1.0f, -1.0f), spk::Vector2(1.0f, -1.0f), spk::Vector2(1.0f, 1.0f), spk::Vector2(-1.0f, 1.0f));

	EXPECT_NEAR(square.area(), 4.0f, 1e-5f);
	EXPECT_NEAR(square.perimeter(), 8.0f, 1e-5f);
	EXPECT_EQ(square.centroid(), spk::Vector2(0.0f, 0.0f));
}

TEST_F(Polygon2DTest, ContainmentAndAdjacency)
{
	spk::Polygon2D outer =
		spk::Polygon2D::makeSquare(spk::Vector2(0.0f, 0.0f), spk::Vector2(2.0f, 0.0f), spk::Vector2(2.0f, 2.0f), spk::Vector2(0.0f, 2.0f));
	spk::Polygon2D inner =
		spk::Polygon2D::makeSquare(spk::Vector2(0.5f, 0.5f), spk::Vector2(1.5f, 0.5f), spk::Vector2(1.5f, 1.5f), spk::Vector2(0.5f, 1.5f));
	spk::Polygon2D neighbor =
		spk::Polygon2D::makeSquare(spk::Vector2(2.0f, 0.0f), spk::Vector2(4.0f, 0.0f), spk::Vector2(4.0f, 2.0f), spk::Vector2(2.0f, 2.0f));

	EXPECT_TRUE(outer.contains(spk::Vector2(1.0f, 1.0f)));
	EXPECT_FALSE(outer.contains(spk::Vector2(-0.1f, 1.0f)));
	EXPECT_TRUE(outer.contains(inner));
	EXPECT_TRUE(outer.isAdjacent(neighbor));
	EXPECT_TRUE(outer.isConvex());
}

TEST_F(Polygon2DTest, OverlapSequantAndTriangulate)
{
	spk::Polygon2D concave = spk::Polygon2D::fromLoop({
		spk::Vector2(0.0f, 0.0f),
		spk::Vector2(2.0f, 0.0f),
		spk::Vector2(2.0f, 1.0f),
		spk::Vector2(1.0f, 1.0f),
		spk::Vector2(1.0f, 2.0f),
		spk::Vector2(0.0f, 2.0f),
	});
	spk::Polygon2D other =
		spk::Polygon2D::makeSquare(spk::Vector2(1.0f, 0.5f), spk::Vector2(2.5f, 0.5f), spk::Vector2(2.5f, 1.5f), spk::Vector2(1.0f, 1.5f));

	EXPECT_FALSE(concave.isConvex());
	EXPECT_TRUE(concave.isOverlapping(other));
	EXPECT_TRUE(concave.isSequant(other));

	std::vector<spk::Polygon2D> triangles = concave.triangulate();
	float totalArea =
		std::accumulate(triangles.begin(), triangles.end(), 0.0f, [](float p_acc, const spk::Polygon2D &p_tri) { return p_acc + p_tri.area(); });
	EXPECT_NEAR(totalArea, concave.area(), 1e-4f);

	std::vector<spk::Polygon2D> convexParts = concave.splitIntoConvex();
	EXPECT_GE(convexParts.size(), 2U);
	for (const auto &poly : convexParts)
	{
		EXPECT_TRUE(poly.isConvex());
	}
}
