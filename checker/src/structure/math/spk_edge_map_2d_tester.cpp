#include "structure/math/spk_edge_map_2d_tester.hpp"

#include "structure/math/spk_polygon_2d.hpp"

TEST_F(EdgeMap2DTest, ConstructUnionFromTriangles)
{
	spk::Polygon2D triA = spk::Polygon2D::makeTriangle(spk::Vector2(0.0f, 0.0f), spk::Vector2(1.0f, 0.0f), spk::Vector2(0.0f, 1.0f));
	spk::Polygon2D triB = spk::Polygon2D::makeTriangle(spk::Vector2(1.0f, 0.0f), spk::Vector2(1.0f, 1.0f), spk::Vector2(0.0f, 1.0f));

	spk::EdgeMap2D map;
	map.addPolygon(triA);
	map.addPolygon(triB);

	std::vector<spk::Polygon2D> result = map.construct();
	ASSERT_EQ(result.size(), 1U);

	const spk::Polygon2D &merged = result.front();
	EXPECT_NEAR(merged.area(), 1.0f, 1e-4f);
	EXPECT_TRUE(merged.contains(spk::Vector2(0.5f, 0.5f)));
	EXPECT_FALSE(merged.contains(spk::Vector2(1.5f, 0.5f)));
}

TEST_F(EdgeMap2DTest, HandlesDisjointPolygons)
{
	spk::Polygon2D left =
		spk::Polygon2D::makeSquare(spk::Vector2(-1.0f, -1.0f), spk::Vector2(0.0f, -1.0f), spk::Vector2(0.0f, 0.0f), spk::Vector2(-1.0f, 0.0f));
	spk::Polygon2D right =
		spk::Polygon2D::makeSquare(spk::Vector2(1.0f, -1.0f), spk::Vector2(2.0f, -1.0f), spk::Vector2(2.0f, 0.0f), spk::Vector2(1.0f, 0.0f));

	spk::EdgeMap2D map;
	map.addPolygon(left);
	map.addPolygon(right);

	std::vector<spk::Polygon2D> result = map.construct();
	ASSERT_EQ(result.size(), 2U);
	const bool leftContained = result[0].contains(spk::Vector2(-0.5f, -0.5f)) || result[1].contains(spk::Vector2(-0.5f, -0.5f));
	const bool rightContained = result[0].contains(spk::Vector2(1.5f, -0.5f)) || result[1].contains(spk::Vector2(1.5f, -0.5f));
	EXPECT_TRUE(leftContained);
	EXPECT_TRUE(rightContained);
}
