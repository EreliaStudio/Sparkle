#include "structure/math/spk_edge_map_tester.hpp"

#include "structure/math/spk_polygon.hpp"

TEST_F(EdgeMapTest, ConstructUnionFromTriangles)
{
	spk::Polygon triA = spk::Polygon::makeTriangle(spk::Vector3(0.0f, 0.0f, 0.0f), spk::Vector3(1.0f, 0.0f, 0.0f), spk::Vector3(0.0f, 1.0f, 0.0f));
	spk::Polygon triB = spk::Polygon::makeTriangle(spk::Vector3(1.0f, 0.0f, 0.0f), spk::Vector3(1.0f, 1.0f, 0.0f), spk::Vector3(0.0f, 1.0f, 0.0f));

	spk::EdgeMap map;
	map.addPolygon(triA);
	map.addPolygon(triB);

	std::vector<spk::Polygon> result = map.construct();
	ASSERT_EQ(result.size(), 1U);

	const spk::Polygon &merged = result.front();
	EXPECT_TRUE(merged.contains(spk::Vector3(0.5f, 0.5f, 0.0f)));
	EXPECT_FALSE(merged.contains(spk::Vector3(1.5f, 0.5f, 0.0f)));
	EXPECT_TRUE(merged.isPlanar());
	EXPECT_TRUE(merged.isConvex());
}

TEST_F(EdgeMapTest, FiltersContainedLoops)
{
	spk::Polygon outer = spk::Polygon::makeSquare(
		spk::Vector3(-1.0f, -1.0f, 0.0f), spk::Vector3(1.0f, -1.0f, 0.0f), spk::Vector3(1.0f, 1.0f, 0.0f), spk::Vector3(-1.0f, 1.0f, 0.0f));
	spk::Polygon inner = spk::Polygon::makeSquare(
		spk::Vector3(-0.5f, -0.5f, 0.0f), spk::Vector3(0.5f, -0.5f, 0.0f), spk::Vector3(0.5f, 0.5f, 0.0f), spk::Vector3(-0.5f, 0.5f, 0.0f));

	spk::EdgeMap map;
	map.addPolygon(outer);
	map.addPolygon(inner);

	std::vector<spk::Polygon> result = map.construct();
	ASSERT_EQ(result.size(), 1U);
	EXPECT_TRUE(result.front().contains(spk::Vector3(0.75f, 0.0f, 0.0f)));
	EXPECT_TRUE(result.front().contains(spk::Vector3(0.0f, 0.0f, 0.0f)));
	EXPECT_EQ(result.front().edges().size(), outer.edges().size());
}
