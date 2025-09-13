#include <gtest/gtest.h>
#include <rapidcheck.h>

#include "structure/math/spk_plane.hpp"
#include "structure/math/spk_vector3.hpp"
#include <algorithm>
#include <vector>

// NOTE: Must be in rc namespace!
namespace rc
{

	template <>
	struct Arbitrary<spk::Vector3>
	{
		static Gen<spk::Vector3> arbitrary()
		{
			return gen::construct<spk::Vector3>(gen::arbitrary<float>(), gen::arbitrary<float>(), gen::arbitrary<float>());
		}
	};

} // namespace rc

TEST(PlaneTester, OriginIsInPlane)
{
	spk::Vector3 origin{1, 1, 1};
	spk::Vector3 normal{1, 1, 2};
	spk::Plane p0{normal, origin};

	GTEST_ASSERT_TRUE(p0.contains(origin));

	GTEST_ASSERT_TRUE(
		rc::check(
			"Plane contains its origin",
			[](const spk::Vector3 &p_normal, const spk::Vector3 &p_origin)
			{
				RC_PRE(p_normal.norm() != 0);

				spk::Plane p{p_normal, p_origin};
				RC_ASSERT(p.contains(p_origin));
				RC_ASSERT_FALSE(p.contains(p_origin + p_normal));
			}));
}

TEST(PlaneTester, PointsAreInPlane)
{
	GTEST_ASSERT_TRUE(
		rc::check(
			"Points in a plane",
			[](spk::Vector3 p_origin, float p_x, float p_y)
			{
				// Since the plane is contructed from the Z unit vector, all its points share the same z.
				spk::Vector3 unitVectorZ{0, 0, 1};
				spk::Plane plane{unitVectorZ, p_origin};
				spk::Vector3 point{p_x, p_y, p_origin.z};

				RC_ASSERT(plane.contains(point));
			}));
}

TEST(PlaneTester, PlanesAreEqual)
{
	spk::Plane p0{spk::Vector3{0, 0, 1}, spk::Vector3{0, 0, 0}};
	spk::Plane p1{spk::Vector3{0, 0, -2}, spk::Vector3{10, 10, 0}};
	GTEST_ASSERT_EQ(p0, p1);

	spk::Plane p2{spk::Vector3{0, 1, 1}, spk::Vector3{0, 0, 0}};
	GTEST_ASSERT_NE(p0, p2);
}