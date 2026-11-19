#include <gtest/gtest.h>

#include <stdexcept>

#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "rendering/engine/camera_component3d.hpp"

TEST(CameraComponent3DTest, UsesOwnerWorldTransformInsteadOfIndependentCameraPose)
{
	spk::Entity3D parent("parent");
	spk::Entity3D child("child", &parent);
	auto &camera = child.addComponent<spk::CameraComponent3D>("camera");
	parent.transform().place({1, 0, 0});
	child.transform().place({0, 2, 3});

	const auto world = camera.viewMatrix() * spk::Vector4{1, 2, 3, 1};
	EXPECT_NEAR(world.x, 0.0f, 0.0001f);
	EXPECT_NEAR(world.y, 0.0f, 0.0001f);
	EXPECT_NEAR(world.z, 0.0f, 0.0001f);
	const auto combined = camera.viewProjectionMatrix();
	const auto expected = camera.projectionMatrix() * camera.viewMatrix();
	for (std::size_t column = 0; column < 4; ++column)
	{
		for (std::size_t row = 0; row < 4; ++row)
		{
			EXPECT_NEAR(combined[column][row], expected[column][row], 0.0001f);
		}
	}
}

TEST(CameraComponent3DTest, RequiresOwnerAndPreservesProjection)
{
	spk::CameraComponent3D camera;
	EXPECT_THROW((void)camera.viewMatrix(), std::logic_error);
	camera.setOrthographic(-2, 2, -2, 2, 0.1f, 20.0f);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Orthographic);
}
