#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "exception.hpp"
#include "math/vector4.hpp"
#include "shadow_map/projection.hpp"

TEST(ShadowProjectionTest, SpotUsesFinitePerspectiveAndLightForwardDirection)
{
	spk::Light::Selection::Entry light;
	light.type = spk::Light::Selection::Type::Spot;
	light.position = {2, 3, 4};
	light.direction = {0, 0, -1};
	light.range = 24.0f;
	light.angle = 0.5f;
	const auto projection = spk::ShadowMap::Projection::spot(light);
	EXPECT_GT(projection.nearPlane, 0.0f);
	EXPECT_FLOAT_EQ(projection.farPlane, 24.0f);
	const auto center = projection.viewProjection * spk::Vector4{2, 3, -2, 1};
	EXPECT_NEAR(center.x / center.w, 0.0f, 0.0001f);
	EXPECT_NEAR(center.y / center.w, 0.0f, 0.0001f);
	EXPECT_GT(center.w, 0.0f);
}

TEST(ShadowProjectionTest, DirectionalTruncatesInfiniteCameraFarPlaneAtShadowDistance)
{
	spk::Camera3D camera;
	camera.setPerspective(1.0f, 1.5f, 0.1f, 100000000.0f);
	spk::Light::Selection::Entry light;
	light.type = spk::Light::Selection::Type::Directional;
	light.direction = {0, -1, 0};
	light.shadowDistance = 30.0f;
	const auto projection = spk::ShadowMap::Projection::directional(light, camera);
	EXPECT_TRUE(std::isfinite(projection.nearPlane));
	EXPECT_TRUE(std::isfinite(projection.farPlane));
	EXPECT_GT(projection.farPlane, projection.nearPlane);
	for (std::size_t col = 0; col < 4; ++col)
	{
		for (std::size_t row = 0; row < 4; ++row)
		{
			EXPECT_TRUE(std::isfinite(projection.viewProjection[col][row]));
		}
	}
}

TEST(ShadowProjectionTest, InvalidLightInputsAreRejected)
{
	spk::Light::Selection::Entry spot;
	spot.type = spk::Light::Selection::Type::Spot;
	spot.direction = {0, 0, 0};
	spot.range = 10.0f;
	spot.angle = 0.5f;
	EXPECT_THROW((void)spk::ShadowMap::Projection::spot(spot), spk::Exception);
	spot.direction = {0, -1, 0};
	spot.angle = std::numeric_limits<float>::infinity();
	EXPECT_THROW((void)spk::ShadowMap::Projection::spot(spot), spk::Exception);

	spk::Camera3D camera;
	spk::Light::Selection::Entry directional;
	directional.type = spk::Light::Selection::Type::Directional;
	directional.direction = {0, -1, 0};
	directional.shadowDistance = 0.0f;
	EXPECT_THROW((void)spk::ShadowMap::Projection::directional(directional, camera), spk::Exception);
}
