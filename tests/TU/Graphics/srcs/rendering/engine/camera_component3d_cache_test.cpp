#include <gtest/gtest.h>

#include <stdexcept>

#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "rendering/engine/camera_component3d.hpp"

namespace
{
	constexpr float Tolerance = 0.0002f;

	void expectIdentity(const spk::Matrix4x4 &actual)
	{
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				EXPECT_NEAR(actual[column][row], column == row ? 1.0f : 0.0f, Tolerance);
			}
		}
	}

	void expectInversePair(const spk::CameraComponent3D &camera)
	{
		expectIdentity(camera.viewMatrix() * camera.inverseViewMatrix());
		expectIdentity(camera.inverseViewMatrix() * camera.viewMatrix());
	}
}

TEST(CameraComponent3DCacheTest, IdentityMatricesAreMutualInverses)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();

	expectIdentity(camera.viewMatrix());
	expectIdentity(camera.inverseViewMatrix());
	expectInversePair(camera);
}

TEST(CameraComponent3DCacheTest, RepeatedAccessesReuseBothCachedMatrices)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	const auto *view = &camera.viewMatrix();
	const auto *inverse = &camera.inverseViewMatrix();

	EXPECT_EQ(&camera.viewMatrix(), view);
	EXPECT_EQ(&camera.inverseViewMatrix(), inverse);
	EXPECT_NE(view, inverse);
}

TEST(CameraComponent3DCacheTest, InverseCanBeReadBeforeView)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	entity.transform().place({2, -3, 4});
	entity.transform().setEulerRotation({15, 25, -35});

	const auto &inverse = camera.inverseViewMatrix();
	const auto origin = inverse * spk::Vector4{0, 0, 0, 1};
	EXPECT_NEAR(origin.x, 2.0f, Tolerance);
	EXPECT_NEAR(origin.y, -3.0f, Tolerance);
	EXPECT_NEAR(origin.z, 4.0f, Tolerance);
	expectInversePair(camera);
}

TEST(CameraComponent3DCacheTest, LocalTranslationInvalidatesBothMatrices)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	const auto beforeView = camera.viewMatrix();
	const auto beforeInverse = camera.inverseViewMatrix();
	entity.transform().place({3, 4, -2});

	EXPECT_NE(camera.viewMatrix()[3][0], beforeView[3][0]);
	EXPECT_NE(camera.inverseViewMatrix()[3][0], beforeInverse[3][0]);
	expectInversePair(camera);
}

TEST(CameraComponent3DCacheTest, LocalRotationInvalidatesBothMatrices)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	const auto beforeView = camera.viewMatrix();
	const auto beforeInverse = camera.inverseViewMatrix();
	entity.transform().setEulerRotation({0, 90, 0});

	EXPECT_NEAR(camera.viewMatrix()[0][2], 1.0f, Tolerance);
	EXPECT_NE(camera.viewMatrix()[0][2], beforeView[0][2]);
	EXPECT_NE(camera.inverseViewMatrix()[0][2], beforeInverse[0][2]);
	expectInversePair(camera);
}

TEST(CameraComponent3DCacheTest, ParentMovementRefreshesCachedPose)
{
	spk::Entity3D parent("Parent");
	spk::Entity3D entity("Camera", &parent);
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	(void)camera.viewMatrix();
	(void)camera.inverseViewMatrix();
	parent.transform().place({5, 0, -2});

	EXPECT_NEAR(camera.inverseViewMatrix()[3][0], 5.0f, Tolerance);
	EXPECT_NEAR(camera.inverseViewMatrix()[3][2], -2.0f, Tolerance);
	expectInversePair(camera);
}

TEST(CameraComponent3DCacheTest, ParentRotationRefreshesCachedPose)
{
	spk::Entity3D parent("Parent");
	spk::Entity3D entity("Camera", &parent);
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	entity.transform().place({0, 0, 2});
	const auto before = camera.inverseViewMatrix();
	parent.transform().setEulerRotation({0, 90, 0});

	EXPECT_NE(camera.inverseViewMatrix()[3][0], before[3][0]);
	expectInversePair(camera);
}

TEST(CameraComponent3DCacheTest, ReparentingRefreshesCachedPose)
{
	spk::Entity3D first("First");
	spk::Entity3D second("Second");
	spk::Entity3D entity("Camera", &first);
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	first.transform().place({1, 0, 0});
	second.transform().place({8, 0, 0});
	const auto before = camera.inverseViewMatrix();
	entity.setParent(second);

	EXPECT_NE(camera.inverseViewMatrix()[3][0], before[3][0]);
	EXPECT_NEAR(camera.inverseViewMatrix()[3][0], 8.0f, Tolerance);
	expectInversePair(camera);
}

TEST(CameraComponent3DCacheTest, ProjectionChangeDoesNotInvalidateViewMatrices)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	entity.transform().place({2, 3, 4});
	const auto view = camera.viewMatrix();
	const auto inverse = camera.inverseViewMatrix();
	const auto *viewAddress = &camera.viewMatrix();
	const auto *inverseAddress = &camera.inverseViewMatrix();
	camera.setOrthographic(-4, 4, -3, 3, 0.1f, 50.0f);

	EXPECT_EQ(&camera.viewMatrix(), viewAddress);
	EXPECT_EQ(&camera.inverseViewMatrix(), inverseAddress);
	EXPECT_EQ(camera.viewMatrix()[3][0], view[3][0]);
	EXPECT_EQ(camera.inverseViewMatrix()[3][0], inverse[3][0]);
}

TEST(CameraComponent3DCacheTest, ScaleDoesNotAffectViewOrInverseView)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	entity.transform().place({1, 2, 3});
	const auto view = camera.viewMatrix();
	const auto inverse = camera.inverseViewMatrix();
	entity.transform().rescale({2, 3, -4});

	EXPECT_EQ(camera.viewMatrix()[3][0], view[3][0]);
	EXPECT_EQ(camera.inverseViewMatrix()[3][0], inverse[3][0]);
	expectInversePair(camera);
}

TEST(CameraComponent3DCacheTest, DetachedCameraNeverReturnsStaleCachedMatrices)
{
	spk::Entity3D entity("Camera");
	spk::CameraComponent3D camera("Camera", &entity);
	(void)camera.viewMatrix();
	(void)camera.inverseViewMatrix();
	camera.attach(nullptr);

	EXPECT_THROW(static_cast<void>(camera.viewMatrix()), std::logic_error);
	EXPECT_THROW(static_cast<void>(camera.inverseViewMatrix()), std::logic_error);
}

TEST(CameraComponent3DCacheTest, ReattachmentUsesNewWorldPose)
{
	spk::Entity3D first("First");
	spk::Entity3D second("Second");
	spk::CameraComponent3D camera("Camera", &first);
	first.transform().place({1, 0, 0});
	second.transform().place({7, 0, 0});
	(void)camera.viewMatrix();
	camera.attach(&second);

	EXPECT_NEAR(camera.inverseViewMatrix()[3][0], 7.0f, Tolerance);
	expectInversePair(camera);
}
