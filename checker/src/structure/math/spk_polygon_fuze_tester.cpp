#include "structure/math/spk_polygon_fuze_tester.hpp"

TEST_F(PolygonFuzeTest, FuzeAdjacentSquares)
{
	spk::Polygon left = spk::Polygon::makeSquare(
		spk::Vector3(-1.0f, -1.0f, 0.0f), spk::Vector3(0.0f, -1.0f, 0.0f), spk::Vector3(0.0f, 1.0f, 0.0f), spk::Vector3(-1.0f, 1.0f, 0.0f));
	spk::Polygon right = spk::Polygon::makeSquare(
		spk::Vector3(0.0f, -1.0f, 0.0f), spk::Vector3(1.0f, -1.0f, 0.0f), spk::Vector3(1.0f, 1.0f, 0.0f), spk::Vector3(0.0f, 1.0f, 0.0f));

	spk::Polygon fused = left.fuze(right, true);
	EXPECT_TRUE(fused.contains(spk::Vector3(-0.5f, 0.0f, 0.0f)));
	EXPECT_TRUE(fused.contains(spk::Vector3(0.5f, 0.0f, 0.0f)));
	EXPECT_FALSE(fused.contains(spk::Vector3(1.5f, 0.0f, 0.0f)));
	EXPECT_TRUE(fused.isConvex());
}

TEST_F(PolygonFuzeTest, FuzeGroupCombinesAll)
{
	spk::Polygon a = spk::Polygon::makeSquare(
		spk::Vector3(0.0f, 0.0f, 0.0f), spk::Vector3(1.0f, 0.0f, 0.0f), spk::Vector3(1.0f, 1.0f, 0.0f), spk::Vector3(0.0f, 1.0f, 0.0f));
	spk::Polygon b = spk::Polygon::makeSquare(
		spk::Vector3(1.0f, 0.0f, 0.0f), spk::Vector3(2.0f, 0.0f, 0.0f), spk::Vector3(2.0f, 1.0f, 0.0f), spk::Vector3(1.0f, 1.0f, 0.0f));
	spk::Polygon c = spk::Polygon::makeSquare(
		spk::Vector3(0.0f, 1.0f, 0.0f), spk::Vector3(1.0f, 1.0f, 0.0f), spk::Vector3(1.0f, 2.0f, 0.0f), spk::Vector3(0.0f, 2.0f, 0.0f));

	spk::Polygon fused = spk::Polygon::fuzeGroup({a, b, c});
	EXPECT_TRUE(fused.contains(spk::Vector3(0.5f, 0.5f, 0.0f)));
	EXPECT_TRUE(fused.contains(spk::Vector3(1.5f, 0.5f, 0.0f)));
	EXPECT_TRUE(fused.contains(spk::Vector3(0.5f, 1.5f, 0.0f)));
	EXPECT_FALSE(fused.contains(spk::Vector3(1.5f, 1.5f, 0.0f)));
}

TEST_F(PolygonFuzeTest, NonCoplanarCheckRaises)
{
	spk::Polygon base = spk::Polygon::makeSquare(
		spk::Vector3(0.0f, 0.0f, 0.0f), spk::Vector3(1.0f, 0.0f, 0.0f), spk::Vector3(1.0f, 1.0f, 0.0f), spk::Vector3(0.0f, 1.0f, 0.0f));
	spk::Polygon tilted = spk::Polygon::makeSquare(
		spk::Vector3(0.0f, 0.0f, 1.0f), spk::Vector3(1.0f, 0.0f, 1.0f), spk::Vector3(1.0f, 1.0f, 1.0f), spk::Vector3(0.0f, 1.0f, 1.0f));

	EXPECT_THROW(base.fuze(tilted, true), std::runtime_error);
}
