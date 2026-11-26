#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>
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

TEST(ShadowProjectionTest, PointFacesFollowOpenGLOrientationWithNinetyDegreeFrusta)
{
	spk::Light::Selection::Entry light;
	light.type = spk::Light::Selection::Type::Point;
	light.position = {2, 3, 4};
	light.range = 12.0f;
	const auto faces = spk::ShadowMap::Projection::point(light);
	constexpr std::array<spk::Vector3, 6> axes{{{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
	constexpr std::array<spk::Vector3, 6> rights{{{0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {-1, 0, 0}}};
	constexpr std::array<spk::Vector3, 6> ups{{{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}}};
	for (std::size_t index = 0; index < faces.size(); ++index)
	{
		const auto &projection = faces[index];
		EXPECT_FLOAT_EQ(projection.nearPlane, 0.012f);
		EXPECT_FLOAT_EQ(projection.farPlane, 12.0f);
		const auto projected = [&](spk::Vector3 direction) {
			const spk::Vector3 p = light.position + direction * spk::Vector3{4, 4, 4};
			return projection.viewProjection * spk::Vector4{p.x, p.y, p.z, 1};
		};
		const auto center = projected(axes[index]);
		EXPECT_GT(center.w, 0.0f);
		EXPECT_NEAR(center.x / center.w, 0.0f, 0.0001f);
		EXPECT_NEAR(center.y / center.w, 0.0f, 0.0001f);
		const auto right = projected(axes[index] + rights[index]);
		const auto up = projected(axes[index] + ups[index]);
		EXPECT_NEAR(right.x / right.w, 1.0f, 0.0001f);
		EXPECT_NEAR(up.y / up.w, 1.0f, 0.0001f);
	}
}

TEST(ShadowProjectionTest, PointRejectsZeroRangeAndNonFiniteInput)
{
	spk::Light::Selection::Entry light;
	light.type = spk::Light::Selection::Type::Point;
	light.range = 0.0f;
	EXPECT_THROW((void)spk::ShadowMap::Projection::point(light), spk::Exception);
	light.range = 10.0f;
	light.position.x = std::numeric_limits<float>::quiet_NaN();
	EXPECT_THROW((void)spk::ShadowMap::Projection::point(light), spk::Exception);
	light.position = {0, 0, 0};
	light.range = std::numeric_limits<float>::infinity();
	EXPECT_THROW((void)spk::ShadowMap::Projection::point(light), spk::Exception);
}

TEST(ShadowProjectionTest, CascadesIncludeOffscreenBlockersAndRejectUnrelatedCasters)
{
	spk::Camera3D camera;
	camera.setPosition({0, 10, 8});
	camera.lookAt({0, -0.2f, 0});
	camera.setOrthographic(-4, 4, -3, 3, 0.1f, 18);
	spk::Light::Selection::Entry light;
	light.type = spk::Light::Selection::Type::Directional;
	light.direction = {3, -1, 0};
	light.shadowDistance = 18;
	light.shadowResolution = 2048;
	const std::array<spk::ShadowMap::CasterBounds, 2> bounds{{
		{{-19, -0.2f, -1}, {-17, 7.8f, 1}},
		{{100, 0, 100}, {102, 8, 102}}}};
	const auto cascades = spk::ShadowMap::Projection::directionalCascades(light, camera, bounds);
	const auto receiverDepth = -(camera.viewMatrix() * spk::Vector4{0, -0.2f, 0, 1}).z;
	std::size_t index = 0;
	while (index + 1 < cascades.size() && receiverDepth > cascades[index].splitFar)
	{
		++index;
	}
	EXPECT_TRUE(cascades[index].intersects(bounds[0]));
	EXPECT_FALSE(cascades[index].intersects(bounds[1]));
	for (std::size_t i = 1; i < cascades.size(); ++i)
	{
		EXPECT_GT(cascades[i].splitFar, cascades[i - 1].splitFar);
	}
	EXPECT_NEAR(cascades.back().splitFar, 18.0f, 0.001f);
}

TEST(ShadowProjectionTest, NearPerspectiveCascadesHaveHigherSpatialPrecision)
{
	spk::Camera3D camera;
	camera.setPerspective(0.9f, 4.0f / 3.0f, 0.1f, 240.0f);
	spk::Light::Selection::Entry light;
	light.type = spk::Light::Selection::Type::Directional;
	light.direction = {36, -75, -20};
	light.shadowDistance = 160;
	light.shadowResolution = 2048;
	const auto cascades = spk::ShadowMap::Projection::directionalCascades(light, camera, {});
	for (std::size_t i = 1; i < cascades.size(); ++i)
	{
		EXPECT_GT(cascades[i].worldPerTexel, cascades[i - 1].worldPerTexel);
	}
	EXPECT_LT(cascades.front().worldPerTexel, cascades.back().worldPerTexel * 0.25f);
	for (const auto &cascade : cascades)
	{
		EXPECT_GT(cascade.farPlane, cascade.nearPlane);
		EXPECT_GT(cascade.worldPerTexel, 0.0f);
	}
}

TEST(ShadowProjectionPerformance, Randomized128CastersHardBudget)
{
	using Clock = std::chrono::steady_clock;
	constexpr std::size_t Cases = 128;
	constexpr std::size_t CasterCount = 128;
	constexpr double BudgetMs = 2.0;
	std::mt19937 random(0x5A4D0u);
	const auto sample = [&]() {
		return static_cast<float>(random() % 2001) / 1000.0f - 1.0f;
	};
	std::vector<double> medians;
	medians.reserve(Cases);
	std::size_t selected = 0;
	std::size_t overBudget = 0;
	for (std::size_t index = 0; index < Cases; ++index)
	{
		spk::Camera3D camera;
		const spk::Vector3 eye{sample() * 40, 30 + sample() * 10, sample() * 40};
		camera.setPosition(eye);
		camera.lookAt({0, 0, 0});
		camera.setPerspective(0.9f, 4.0f / 3.0f, 0.1f, 240);
		spk::Light::Selection::Entry light;
		light.type = spk::Light::Selection::Type::Directional;
		light.direction = {sample(), -0.2f - std::abs(sample()), sample()};
		light.shadowDistance = 160;
		light.shadowResolution = 2048;
		std::array<spk::ShadowMap::CasterBounds, CasterCount> casters{};
		for (auto &caster : casters)
		{
			caster.minimum = {sample() * 100, sample() * 20, sample() * 100};
			caster.maximum = caster.minimum + spk::Vector3{16, 16, 16};
		}
		std::array<double, 7> times{};
		for (auto &time : times)
		{
			const auto begin = Clock::now();
			const auto cascades = spk::ShadowMap::Projection::directionalCascades(light, camera, casters);
			for (const auto &cascade : cascades)
			{
				for (const auto &caster : casters)
				{
					selected += cascade.intersects(caster) ? 1u : 0u;
				}
			}
			time = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
		}
		std::sort(times.begin(), times.end());
		medians.push_back(times[times.size() / 2]);
		overBudget += medians.back() > BudgetMs ? 1u : 0u;
	}
	std::sort(medians.begin(), medians.end());
	std::cout << "[Directional projection stress] cases=" << Cases << " casters=" << CasterCount
			  << " median=" << medians[Cases / 2] << "ms p95=" << medians[Cases * 95 / 100]
			  << "ms p99=" << medians[Cases * 99 / 100] << "ms worst=" << medians.back()
			  << "ms over_budget=" << overBudget << " budget=" << BudgetMs << "ms\n";
	EXPECT_GT(selected, 0u);
	EXPECT_LT(selected, Cases * 7 * CasterCount * spk::ShadowMap::Projection::CascadeCount);
#if defined(NDEBUG)
	EXPECT_EQ(overBudget, 0u) << "Optimize projection and caster selection before raising the budget";
#endif
}
