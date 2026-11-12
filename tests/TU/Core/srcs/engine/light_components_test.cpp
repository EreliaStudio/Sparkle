#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <vector>

#include "engine/engine_headless.hpp"
#include "engine/entity3d.hpp"
#include "engine/reference_frame.hpp"
#include "engine/registry.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "light/contributor.hpp"
#include "light/directional.hpp"
#include "light/frustum.hpp"
#include "light/point.hpp"
#include "light/spot.hpp"
#include "math/camera_3d.hpp"

namespace
{
	[[nodiscard]] spk::Light::Frustum visibleFrustum()
	{
		spk::Camera3D camera;
		camera.setPerspective(std::numbers::pi_v<float> / 2.0f, 1.0f, 1.0f, 10.0f);
		return spk::Light::Frustum::fromViewProjection(camera.viewProjectionMatrix());
	}

	constexpr float NaN = std::numeric_limits<float>::quiet_NaN();
	constexpr float Infinity = std::numeric_limits<float>::infinity();
}

TEST(CoreLightTest, DefaultValuesAndIndependentShadowSettings)
{
	spk::Entity3D entity("lights");
	auto &point = entity.addComponent<spk::Light::Point>();
	auto &spot = entity.addComponent<spk::Light::Spot>();
	auto &directional = entity.addComponent<spk::Light::Directional>();

	for (const spk::Light::Contributor *light :
		 {static_cast<spk::Light::Contributor *>(&point),
		  static_cast<spk::Light::Contributor *>(&spot),
		  static_cast<spk::Light::Contributor *>(&directional)})
	{
		EXPECT_EQ(light->owner(), &entity);
		EXPECT_EQ(light->color(), (spk::RGB{1.0f, 1.0f, 1.0f}));
		EXPECT_FLOAT_EQ(light->intensity(), 1.0f);
		EXPECT_FLOAT_EQ(light->priorityWeight(), 1.0f);
		EXPECT_EQ(light->shadowResolution(), 1024u);
		EXPECT_EQ(light->registrationOrder(), 0u);
	}
	EXPECT_FALSE(point.castsShadows());
	EXPECT_TRUE(spot.castsShadows());
	EXPECT_TRUE(directional.castsShadows());
	EXPECT_EQ(point.range(), 10.0f);
	EXPECT_EQ(spot.range(), 10.0f);
	EXPECT_FLOAT_EQ(spot.angle(), 0.5235987756f);
	EXPECT_EQ(directional.shadowDistance(), 100.0f);

	point.setShadowResolution(512);
	point.setCastsShadows(true);
	EXPECT_EQ(point.shadowResolution(), 512u);
	EXPECT_EQ(spot.shadowResolution(), 1024u);
	EXPECT_EQ(directional.shadowResolution(), 1024u);
	EXPECT_TRUE(point.castsShadows());
}

TEST(CoreLightTest, InvalidSharedParametersPreserveStateAndNoIntensityCeiling)
{
	spk::Light::Point point;
	const spk::RGB previousColor = point.color();
	for (spk::RGB color : {spk::RGB{-0.01f, 0.0f, 0.0f}, spk::RGB{0.0f, 1.01f, 0.0f}, spk::RGB{0.0f, 0.0f, Infinity}, spk::RGB{NaN, 0.0f, 0.0f}})
	{
		EXPECT_THROW(point.setColor(color), spk::Exception);
		EXPECT_EQ(point.color(), previousColor);
	}
	point.setColor({0.2f, 0.4f, 0.6f});
	EXPECT_EQ(point.color(), (spk::RGB{0.2f, 0.4f, 0.6f}));

	point.setIntensity(1.0e30f);
	EXPECT_FLOAT_EQ(point.intensity(), 1.0e30f);
	for (float value : {-1.0f, NaN, Infinity, -Infinity})
	{
		EXPECT_THROW(point.setIntensity(value), spk::Exception);
		EXPECT_FLOAT_EQ(point.intensity(), 1.0e30f);
		EXPECT_THROW(point.setPriorityWeight(value), spk::Exception);
		EXPECT_FLOAT_EQ(point.priorityWeight(), 1.0f);
	}
	point.setPriorityWeight(0.0f);
	EXPECT_FLOAT_EQ(point.priorityWeight(), 0.0f);
	for (std::uint32_t resolution : {0u, std::numeric_limits<std::uint32_t>::max()})
	{
		EXPECT_THROW(point.setShadowResolution(resolution), spk::Exception);
		EXPECT_EQ(point.shadowResolution(), 1024u);
	}
}

TEST(CoreLightTest, RangeAndAngleSettersPreserveLocalGeometryOnFailure)
{
	spk::Light::Point point;
	spk::Light::Spot spot;
	spot.setRange(4.0f);
	const float initialAngle = spot.angle();
	const float initialRadius = spot.boundingSphereRadius();
	const float initialBase = spot.baseRadius();
	for (float range : {-1.0f, NaN, Infinity})
	{
		EXPECT_THROW(point.setRange(range), spk::Exception);
		EXPECT_EQ(point.range(), 10.0f);
		EXPECT_THROW(spot.setRange(range), spk::Exception);
		EXPECT_EQ(spot.range(), 4.0f);
		EXPECT_FLOAT_EQ(spot.boundingSphereRadius(), initialRadius);
	}
	for (float angle : {-0.1f, 0.0f, std::numbers::pi_v<float> / 2.0f, NaN, Infinity})
	{
		EXPECT_THROW(spot.setAngle(angle), spk::Exception);
		EXPECT_FLOAT_EQ(spot.angle(), initialAngle);
		EXPECT_FLOAT_EQ(spot.baseRadius(), initialBase);
		EXPECT_FLOAT_EQ(spot.boundingSphereRadius(), initialRadius);
	}
	spot.setRange(0.0f);
	EXPECT_FLOAT_EQ(spot.boundingSphereRadius(), 0.0f);
	EXPECT_FLOAT_EQ(spot.baseRadius(), 0.0f);
	spot.setRange(0.0001f);
	EXPECT_GT(spot.boundingSphereRadius(), 0.0f);
	EXPECT_GT(spot.baseRadius(), 0.0f);
	point.setRange(0.0001f);
	EXPECT_FLOAT_EQ(point.attenuation(0.00005f), 0.5f);
}

TEST(CoreLightTest, RadialSmoothstepAndAngularSmoothstep)
{
	spk::Entity3D entity("spot");
	auto &point = entity.addComponent<spk::Light::Point>();
	auto &spot = entity.addComponent<spk::Light::Spot>();
	point.setRange(4.0f);
	spot.setRange(4.0f);

	EXPECT_FLOAT_EQ(point.attenuation(0.0f), 1.0f);
	EXPECT_FLOAT_EQ(point.attenuation(2.0f), 0.5f);
	EXPECT_FLOAT_EQ(point.attenuation(4.0f), 0.0f);
	EXPECT_FLOAT_EQ(point.attenuation(5.0f), 0.0f);
	EXPECT_THROW(static_cast<void>(point.attenuation(-1.0f)), spk::Exception);
	EXPECT_FLOAT_EQ(spot.attenuation(2.0f, {0.0f, 0.0f, -1.0f}), 0.5f);
	EXPECT_NEAR(spot.angularAttenuation({0.0f, 0.0f, -1.0f}), 1.0f, 1e-6f);
	EXPECT_NEAR(spot.angularAttenuation({1.0f, 0.0f, 0.0f}), 0.0f, 1e-6f);
	const float angle = spot.angle();
	EXPECT_NEAR(spot.angularAttenuation({std::sin(angle), 0.0f, -std::cos(angle)}), 0.0f, 1e-5f);

	point.setRange(0.0f);
	spot.setRange(0.0f);
	EXPECT_FLOAT_EQ(point.attenuation(0.0f), 0.0f);
	EXPECT_FLOAT_EQ(spot.attenuation(0.0f, {0.0f, 0.0f, -1.0f}), 0.0f);
}

TEST(CoreLightTest, NormalizedFrustumCullsSphereAndFiniteCone)
{
	const auto frustum = visibleFrustum();
	for (const auto &plane : frustum.planes())
	{
		EXPECT_NEAR(plane.normal.length(), 1.0, 1e-5);
	}
	EXPECT_TRUE(frustum.intersectsSphere({0.0f, 0.0f, -5.0f}, 1.0f));
	EXPECT_TRUE(frustum.intersectsSphere({6.4f, 0.0f, -5.0f}, 1.0f));
	EXPECT_FALSE(frustum.intersectsSphere({6.5f, 0.0f, -5.0f}, 1.0f));
	EXPECT_TRUE(frustum.intersectsSphere({0.0f, 0.0f, -0.95f}, 0.1f));
	EXPECT_FALSE(frustum.intersectsSphere({25.0f, 0.0f, -5.0f}, 1.0f));

	// An identity clip matrix describes the six inward planes of [-1, 1]^3.
	const auto unitCube = spk::Light::Frustum::fromViewProjection(spk::Matrix4x4::identity());
	const std::array<spk::Vector3, 6> normals = {
		spk::Vector3{1.0f, 0.0f, 0.0f},
		spk::Vector3{-1.0f, 0.0f, 0.0f},
		spk::Vector3{0.0f, 1.0f, 0.0f},
		spk::Vector3{0.0f, -1.0f, 0.0f},
		spk::Vector3{0.0f, 0.0f, 1.0f},
		spk::Vector3{0.0f, 0.0f, -1.0f}};
	for (std::size_t i = 0; i < normals.size(); ++i)
	{
		EXPECT_EQ(unitCube.planes()[i].normal, normals[i]);
		EXPECT_FLOAT_EQ(unitCube.planes()[i].offset, 1.0f);
	}

	spk::Entity3D entity("cone");
	auto &spot = entity.addComponent<spk::Light::Spot>();
	spot.setRange(4.0f);
	spot.setAngle(0.5f);
	entity.transform().place({6.0f, 0.0f, -5.0f});
	entity.transform().setEulerRotation({0.0f, -90.0f, 0.0f});
	EXPECT_TRUE(frustum.intersectsSphere(spot.boundingSphereCenter(), spot.boundingSphereRadius()));
	EXPECT_FALSE(spot.intersects(frustum));
	entity.transform().setEulerRotation({0.0f, 90.0f, 0.0f});
	EXPECT_TRUE(spot.intersects(frustum));
}

TEST(CoreLightTest, PriorityScoresHonorRangeIntensityColorAndVisibility)
{
	const auto frustum = visibleFrustum();
	spk::Entity3D entity("point");
	entity.transform().place({0.0f, 0.0f, -5.0f});
	auto &point = entity.addComponent<spk::Light::Point>();
	point.setRange(2.0f);
	const spk::Vector3 camera{0.0f, 0.0f, 0.0f};
	const float initial = point.priorityScore(frustum, camera);
	EXPECT_NEAR(initial, 0.8f, 1e-5f);
	point.setIntensity(2.0f);
	EXPECT_NEAR(point.priorityScore(frustum, camera), 2.0f * initial, 1e-5f);
	point.setIntensity(1.0f);
	point.setRange(4.0f);
	EXPECT_NEAR(point.priorityScore(frustum, camera), 4.0f * initial, 1e-5f);
	point.setColor({0.0f, 0.0f, 0.0f});
	EXPECT_NEAR(point.priorityScore(frustum, camera), 4.0f * initial, 1e-5f);
	point.setPriorityWeight(2.0f);
	EXPECT_NEAR(point.priorityScore(frustum, camera), 8.0f * initial, 1e-5f);
	EXPECT_TRUE(std::isfinite(point.priorityScore(frustum, entity.transform().position())));
	EXPECT_THROW(static_cast<void>(point.priorityScore(frustum, camera, 0.0f)), spk::Exception);

	point.deactivate();
	EXPECT_FLOAT_EQ(point.priorityScore(frustum, camera), 0.0f);
	point.activate();
	point.setRange(0.0f);
	EXPECT_FLOAT_EQ(point.priorityScore(frustum, camera), 0.0f);
	point.setRange(2.0f);
	point.setIntensity(0.0f);
	EXPECT_FLOAT_EQ(point.priorityScore(frustum, camera), 0.0f);
	point.setIntensity(1.0f);
	entity.transform().place({30.0f, 0.0f, -5.0f});
	EXPECT_FLOAT_EQ(point.priorityScore(frustum, camera), 0.0f);

	auto &spot = entity.addComponent<spk::Light::Spot>();
	spot.setRange(2.0f);
	EXPECT_FLOAT_EQ(spot.priorityScore(frustum, camera), 0.0f);
	entity.transform().place({0.0f, 0.0f, -5.0f});
	const float spotScore = spot.priorityScore(frustum, camera);
	EXPECT_GT(spotScore, 0.0f);
	spot.setRange(4.0f);
	EXPECT_NEAR(spot.priorityScore(frustum, camera), 4.0f * spotScore, 1e-5f);
	spot.setIntensity(2.0f);
	EXPECT_NEAR(spot.priorityScore(frustum, camera), 8.0f * spotScore, 1e-5f);
}

TEST(CoreLightTest, DirectionalTransformAndShadowCoverageRemainIndependent)
{
	spk::Entity3D parent("parent");
	spk::Entity3D child("child", &parent);
	auto &sun = child.addComponent<spk::Light::Directional>();
	EXPECT_NEAR(sun.worldDirection().z, -1.0f, 1e-6f);
	parent.transform().setEulerRotation({0.0f, 90.0f, 0.0f});
	EXPECT_NEAR(sun.worldDirection().x, -1.0f, 1e-6f);
	EXPECT_NEAR(sun.worldDirection().z, 0.0f, 1e-6f);
	EXPECT_FLOAT_EQ(sun.attenuation(), 1.0f);
	sun.setShadowDistance(5.0f);
	EXPECT_FLOAT_EQ(sun.attenuation(), 1.0f);
	for (float invalid : {-1.0f, Infinity, NaN})
	{
		EXPECT_THROW(sun.setShadowDistance(invalid), spk::Exception);
		EXPECT_FLOAT_EQ(sun.shadowDistance(), 5.0f);
	}
}

TEST(CoreLightTest, ParentTransformsMoveAndRotateLocalLights)
{
	spk::Entity3D parent("parent");
	spk::Entity3D child("child", &parent);
	parent.transform().place({10.0f, 2.0f, -3.0f});
	child.transform().place({0.0f, 0.0f, -2.0f});
	auto &point = child.addComponent<spk::Light::Point>();
	auto &spot = child.addComponent<spk::Light::Spot>();
	EXPECT_EQ(point.worldPosition(), (spk::Vector3{10.0f, 2.0f, -5.0f}));
	EXPECT_EQ(spot.worldPosition(), point.worldPosition());
	parent.transform().setEulerRotation({0.0f, 90.0f, 0.0f});
	EXPECT_NEAR(point.worldPosition().x, 8.0f, 1e-5f);
	EXPECT_NEAR(point.worldPosition().z, -3.0f, 1e-5f);
	EXPECT_NEAR(spot.worldDirection().x, -1.0f, 1e-5f);
	parent.transform().move({1.0f, 0.0f, 0.0f});
	EXPECT_NEAR(point.worldPosition().x, 9.0f, 1e-5f);
}

TEST(CoreLightTest, RegistrationOrdinalsAreStableAndEngineLocal)
{
	spk::EngineHeadless engineA;
	spk::EngineHeadless engineB;
	spk::Entity3D first("first");
	spk::Entity3D second("second");
	spk::Entity3D third("third");
	auto &pointA = first.addComponent<spk::Light::Point>();
	auto &spotB = second.addComponent<spk::Light::Spot>();
	auto &sunC = third.addComponent<spk::Light::Directional>();
	engineA.addEntity(&first);
	engineA.addEntity(&second);
	engineA.addEntity(&third);
	EXPECT_EQ(pointA.registrationOrder(), 1u);
	EXPECT_EQ(spotB.registrationOrder(), 2u);
	EXPECT_EQ(sunC.registrationOrder(), 3u);
	const auto &registered = spk::Registry<spk::Light::Contributor, spk::EngineHeadless *>::instance().elements(&engineA);
	EXPECT_EQ(registered.size(), 3u);
	EXPECT_TRUE(registered.contains(&pointA));
	EXPECT_TRUE(registered.contains(&spotB));
	EXPECT_TRUE(registered.contains(&sunC));

	pointA.setIntensity(3.0f);
	first.transform().place({4.0f, 0.0f, -5.0f});
	pointA.deactivate();
	EXPECT_EQ(pointA.registrationOrder(), 1u);
	std::array<const spk::Light::Contributor *, 3> scrambled{&sunC, &pointA, &spotB};
	std::sort(scrambled.begin(), scrambled.end(), [](auto *a, auto *b) {
		return a->registrationOrder() < b->registrationOrder();
	});
	EXPECT_EQ(scrambled[0], &pointA);
	EXPECT_EQ(scrambled[1], &spotB);
	EXPECT_EQ(scrambled[2], &sunC);

	engineA.removeEntity(&first);
	EXPECT_EQ(pointA.registrationOrder(), 0u);
	engineA.addEntity(&first);
	EXPECT_EQ(pointA.registrationOrder(), 4u);
	engineB.addEntity(&first);
	EXPECT_EQ(pointA.registrationOrder(), 1u);
	EXPECT_FALSE(registered.contains(&pointA));
	engineB.removeEntity(&first);
	engineA.addEntity(&first);
	EXPECT_EQ(pointA.registrationOrder(), 5u);

	engineA.removeEntity(&first);
	engineA.removeEntity(&second);
	engineA.removeEntity(&third);
}

TEST(CoreLightTest, OrdinalExhaustionIsCheckedWithoutWrapping)
{
	constexpr std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max();
	EXPECT_EQ(spk::Light::Contributor::nextRegistrationOrder(0), 1u);
	EXPECT_EQ(spk::Light::Contributor::nextRegistrationOrder(maximum - 1), maximum);
	EXPECT_THROW(static_cast<void>(spk::Light::Contributor::nextRegistrationOrder(maximum)), spk::Exception);
}
