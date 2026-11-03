#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

#include "math/camera_3d.hpp"

namespace
{
	void expectMatrix(const spk::Matrix4x4 &actual, const spk::Matrix4x4 &expected)
	{
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				EXPECT_NEAR(actual[column][row], expected[column][row], 0.0001f);
			}
		}
	}
}

TEST(Camera3DTest, ViewIsInverseOfPositionAndOrientation)
{
	spk::Camera3D camera;
	camera.setPosition({1, 2, 5});
	camera.setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	const auto expected = spk::Matrix4x4::rotation(0, 0, -90) *
						  spk::Matrix4x4::translation(-1, -2, -5);
	expectMatrix(camera.viewMatrix(), expected);
	expectMatrix(camera.viewMatrix(), expected);

	camera.setPosition({0, 0, 5});
	const auto translated = spk::Matrix4x4::rotation(0, 0, -90) *
							spk::Matrix4x4::translation(0, 0, -5);
	expectMatrix(camera.viewMatrix(), translated);
}

TEST(Camera3DTest, OrthographicAndPerspectiveProjectionAreIndependentFromPose)
{
	spk::Camera3D camera;
	camera.setOrthographic(-2, 2, -2, 2, 0.1f, 20.0f);
	expectMatrix(camera.projectionMatrix(), spk::Matrix4x4::ortho(-2, 2, -2, 2, 0.1f, 20.0f));
	camera.setPosition({0, 0, 5});
	expectMatrix(camera.viewProjectionMatrix(), camera.projectionMatrix() * camera.viewMatrix());

	camera.setPerspective(1.2f, 1.5f, 0.1f, 20.0f);
	expectMatrix(camera.projectionMatrix(), spk::Matrix4x4::perspective(1.2f, 1.5f, 0.1f, 20.0f));
	EXPECT_THROW(camera.setRotation({0, 0, 0, 0}), std::domain_error);
	EXPECT_THROW(camera.setPosition({0, 0, std::numeric_limits<float>::infinity()}), std::invalid_argument);
}
