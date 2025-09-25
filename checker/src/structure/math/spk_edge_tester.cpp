#include "structure/math/spk_edge_tester.hpp"

#include <unordered_set>

TEST_F(EdgeTest, ConstructorRejectsZeroLength)
{
	EXPECT_THROW(spk::Edge(spk::Vector3(0.0f, 0.0f, 0.0f), spk::Vector3(0.0f, 0.0f, 0.0f)), std::runtime_error);
}

TEST_F(EdgeTest, OrientationContainmentAndProjection)
{
	spk::Edge edge(spk::Vector3(0.0f, 0.0f, 0.0f), spk::Vector3(1.0f, 0.0f, 0.0f));

	float orientation = edge.orientation(spk::Vector3(0.0f, 1.0f, 0.0f), spk::Vector3(0.0f, 0.0f, 1.0f));
	EXPECT_GT(orientation, 0.0f);

	EXPECT_TRUE(edge.contains(spk::Vector3(0.25f, 0.0f, 0.0f), true));
	EXPECT_FALSE(edge.contains(spk::Vector3(0.5f, 0.1f, 0.0f), true));
	EXPECT_TRUE(edge.contains(spk::Vector3(0.5f, 0.1f, 0.0f), false));
	EXPECT_FALSE(edge.contains(spk::Vector3(1.5f, 0.0f, 0.0f), true));

	EXPECT_NEAR(edge.project(spk::Vector3(0.5f, 0.0f, 0.0f)), 0.5f, 1e-6f);
}

TEST_F(EdgeTest, Relations)
{
	spk::Edge base(spk::Vector3(0.0f, 0.0f, 0.0f), spk::Vector3(1.0f, 0.0f, 0.0f));
	spk::Edge forward(spk::Vector3(1.0f, 0.0f, 0.0f), spk::Vector3(2.0f, 0.0f, 0.0f));
	spk::Edge backward(spk::Vector3(1.0f, 0.0f, 0.0f), spk::Vector3(0.0f, 0.0f, 0.0f));
	spk::Edge skew(spk::Vector3(0.0f, 0.0f, 0.0f), spk::Vector3(0.0f, 1.0f, 0.0f));

	EXPECT_TRUE(base.isParallel(forward));
	EXPECT_TRUE(base.isParallel(backward));
	EXPECT_FALSE(base.isParallel(skew));

	EXPECT_TRUE(base.isColinear(forward));
	EXPECT_TRUE(base.isInverse(backward));
	EXPECT_TRUE(base.inverse().isInverse(base));

	EXPECT_TRUE(base.isSame(backward));
	EXPECT_FALSE(base.isSame(skew));

	EXPECT_LT(spk::Edge::Identifier::from(base), spk::Edge::Identifier::from(backward));

	std::unordered_set<spk::Edge> edges;
	edges.insert(base);
	edges.insert(backward);
	EXPECT_EQ(edges.size(), 2U);
}
