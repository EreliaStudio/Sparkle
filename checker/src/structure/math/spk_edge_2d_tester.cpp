#include "structure/math/spk_edge_2d_tester.hpp"

TEST_F(Edge2DTest, OrientationContainmentAndProjection)
{
	spk::Edge2D edge(spk::Vector2(0.0f, 0.0f), spk::Vector2(1.0f, 0.0f));

	EXPECT_GT(edge.orientation(spk::Vector2(0.5f, 1.0f)), 0.0f);
	EXPECT_TRUE(edge.contains(spk::Vector2(0.3f, 0.0f), true));
	EXPECT_FALSE(edge.contains(spk::Vector2(0.3f, 0.1f), true));
	EXPECT_TRUE(edge.contains(spk::Vector2(0.3f, 0.1f), false));
	EXPECT_NEAR(edge.project(spk::Vector2(0.5f, 0.0f)), 0.5f, 1e-6f);
}

TEST_F(Edge2DTest, DegenerateParallelAndColinear)
{
	spk::Edge2D flat(spk::Vector2(0.0f, 0.0f), spk::Vector2(0.0f, 0.0f));
	spk::Edge2D colinear(spk::Vector2(1.0f, 0.0f), spk::Vector2(2.0f, 0.0f));
	spk::Edge2D opposite(spk::Vector2(2.0f, 0.0f), spk::Vector2(1.0f, 0.0f));
	spk::Edge2D vertical(spk::Vector2(0.0f, 0.0f), spk::Vector2(0.0f, 1.0f));

	EXPECT_TRUE(flat.isParallel(spk::Edge2D(spk::Vector2(0.0f, 0.0f), spk::Vector2(0.0f, 0.0f))));
	EXPECT_TRUE(colinear.isParallel(opposite));
	EXPECT_FALSE(colinear.isParallel(vertical));

	EXPECT_TRUE(colinear.isColinear(opposite));
	EXPECT_TRUE(colinear.isInverse(opposite));
	EXPECT_TRUE(colinear.isSame(opposite));
	EXPECT_FALSE(colinear.isSame(vertical));

	spk::Edge2D::Identifier id = spk::Edge2D::Identifier::from(opposite);
	EXPECT_EQ(id.a, spk::Vector2(1.0f, 0.0f));
	EXPECT_EQ(id.b, spk::Vector2(2.0f, 0.0f));
}
