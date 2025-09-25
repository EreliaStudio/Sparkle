#include "structure/math/spk_plane_tester.hpp"

#include "structure/math/spk_polygon.hpp"

TEST_F(PlaneTest, IdentifierCanonicalizesNormal)
{
	spk::Plane plane(spk::Vector3(0.0f, 0.0f, 2.0f), spk::Vector3(0.0f, 0.0f, 1.0f));
	spk::Plane flipped(spk::Vector3(0.0f, 0.0f, -2.0f), spk::Vector3(0.0f, 0.0f, 1.0f));

	spk::Plane::Identifier a = spk::Plane::Identifier::from(plane);
	spk::Plane::Identifier b = spk::Plane::Identifier::from(flipped);

	EXPECT_EQ(a.normal, b.normal);
	EXPECT_FLOAT_EQ(std::abs(a.dotValue), std::abs(b.dotValue));
}

TEST_F(PlaneTest, ContainsAndComparison)
{
	spk::Plane plane(spk::Vector3(0.0f, 0.0f, 1.0f), spk::Vector3(0.0f, 0.0f, 0.0f));
	spk::Polygon square = spk::Polygon::makeSquare(
		spk::Vector3(-1.0f, -1.0f, 0.0f), spk::Vector3(1.0f, -1.0f, 0.0f), spk::Vector3(1.0f, 1.0f, 0.0f), spk::Vector3(-1.0f, 1.0f, 0.0f));

	EXPECT_TRUE(plane.contains(square));
	EXPECT_TRUE(plane.contains(spk::Vector3(0.25f, -0.25f, 0.0f)));
	EXPECT_FALSE(plane.contains(spk::Vector3(0.0f, 0.0f, 0.1f)));

	spk::Plane offset(spk::Vector3(0.0f, 0.0f, -1.0f), spk::Vector3(0.0f, 0.0f, 1.0f));
	EXPECT_FALSE(plane.isSame(offset));
	EXPECT_FALSE(plane == offset);
	EXPECT_NE(plane, offset);
}
