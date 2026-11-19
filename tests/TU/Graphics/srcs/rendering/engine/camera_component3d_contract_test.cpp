#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <stdexcept>

#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "math/camera_3d.hpp"
#include "math/quaternion.hpp"
#include "rendering/engine/camera_component3d.hpp"

namespace
{
	void expectCameraMatrixNear(const spk::Matrix4x4 &actual, const spk::Matrix4x4 &expected)
	{
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				EXPECT_NEAR(actual[column][row], expected[column][row], 0.0001f);
			}
		}
	}

	void expectCameraMatchesOwner(const spk::CameraComponent3D &camera, const spk::Entity3D &entity)
	{
		spk::Camera3D reference;
		reference.setPosition(entity.transform().position(spk::ReferenceFrame::World));
		reference.setRotation(entity.transform().rotation(spk::ReferenceFrame::World));
		expectCameraMatrixNear(camera.viewMatrix(), reference.viewMatrix());
	}
}

TEST(CameraComponent3DContractTest, DefaultProjectionMatchesStandaloneCamera)
{
	spk::Camera3D reference;
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>("Lens");

	EXPECT_EQ(camera.owner(), &entity);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Perspective);
	expectCameraMatrixNear(camera.projectionMatrix(), reference.projectionMatrix());
	expectCameraMatrixNear(camera.viewMatrix(), reference.viewMatrix());
}

TEST(CameraComponent3DContractTest, PerspectiveConfigurationMatchesStandaloneCamera)
{
	spk::Camera3D reference;
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	reference.setPerspective(0.9f, 16.0f / 9.0f, 0.25f, 120.0f);
	camera.setPerspective(0.9f, 16.0f / 9.0f, 0.25f, 120.0f);

	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Perspective);
	expectCameraMatrixNear(camera.projectionMatrix(), reference.projectionMatrix());
}

TEST(CameraComponent3DContractTest, OrthographicConfigurationMatchesStandaloneCamera)
{
	spk::Camera3D reference;
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	reference.setOrthographic(-5, 4, -3, 7, 0.2f, 80.0f);
	camera.setOrthographic(-5, 4, -3, 7, 0.2f, 80.0f);

	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Orthographic);
	expectCameraMatrixNear(camera.projectionMatrix(), reference.projectionMatrix());
}

TEST(CameraComponent3DContractTest, SwitchingProjectionModesPreservesBothConfigurations)
{
	spk::Camera3D reference;
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	camera.setPerspective(0.75f, 1.5f, 0.1f, 50.0f);
	reference.setPerspective(0.75f, 1.5f, 0.1f, 50.0f);
	camera.setOrthographic(-3, 3, -2, 2, 0.1f, 50.0f);
	reference.setOrthographic(-3, 3, -2, 2, 0.1f, 50.0f);

	camera.setProjectionMode(spk::Camera3D::ProjectionMode::Perspective);
	reference.setProjectionMode(spk::Camera3D::ProjectionMode::Perspective);
	expectCameraMatrixNear(camera.projectionMatrix(), reference.projectionMatrix());

	camera.setProjectionMode(spk::Camera3D::ProjectionMode::Orthographic);
	reference.setProjectionMode(spk::Camera3D::ProjectionMode::Orthographic);
	expectCameraMatrixNear(camera.projectionMatrix(), reference.projectionMatrix());
}

TEST(CameraComponent3DContractTest, InvalidProjectionModeLeavesCurrentModeUnchanged)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	camera.setOrthographic(-2, 2, -2, 2, 0.1f, 10.0f);
	const auto previous = camera.projectionMatrix();

	EXPECT_THROW(camera.setProjectionMode(static_cast<spk::Camera3D::ProjectionMode>(255)), std::invalid_argument);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Orthographic);
	expectCameraMatrixNear(camera.projectionMatrix(), previous);
}

TEST(CameraComponent3DContractTest, InvalidPerspectiveParametersLeaveProjectionUnchanged)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	const auto previous = camera.projectionMatrix();

	EXPECT_THROW(camera.setPerspective(0.0f, 1.0f, 0.1f, 10.0f), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(1.0f, 0.0f, 0.1f, 10.0f), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(1.0f, 1.0f, 1.0f, 1.0f), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(std::numeric_limits<float>::quiet_NaN(), 1.0f, 0.1f, 10.0f), std::invalid_argument);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Perspective);
	expectCameraMatrixNear(camera.projectionMatrix(), previous);
}

TEST(CameraComponent3DContractTest, InvalidOrthographicBoundsLeaveProjectionUnchanged)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	const auto previous = camera.projectionMatrix();

	EXPECT_THROW(camera.setOrthographic(2, -2, -2, 2, 0.1f, 10.0f), std::invalid_argument);
	EXPECT_THROW(camera.setOrthographic(-2, 2, 2, 2, 0.1f, 10.0f), std::invalid_argument);
	EXPECT_THROW(camera.setOrthographic(-2, 2, -2, 2, 10.0f, 0.1f), std::invalid_argument);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Perspective);
	expectCameraMatrixNear(camera.projectionMatrix(), previous);
}

TEST(CameraComponent3DContractTest, OwnerPositionIsMappedToViewOrigin)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	entity.transform().place({2, -3, 5});
	const auto origin = camera.viewMatrix() * spk::Vector4{2, -3, 5, 1};

	EXPECT_NEAR(origin.x, 0.0f, 0.0001f);
	EXPECT_NEAR(origin.y, 0.0f, 0.0001f);
	EXPECT_NEAR(origin.z, 0.0f, 0.0001f);
	EXPECT_NEAR(origin.w, 1.0f, 0.0001f);
}

TEST(CameraComponent3DContractTest, RotationUsesOwnerOrientation)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	entity.transform().setEulerRotation({20, 35, -15});
	expectCameraMatchesOwner(camera, entity);
}

TEST(CameraComponent3DContractTest, ParentRotationAndTranslationComposeInWorldFrame)
{
	spk::Entity3D parent("Parent");
	spk::Entity3D entity("Camera", &parent);
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	parent.transform().place({4, -2, 1});
	parent.transform().setEulerRotation({10, 60, 5});
	entity.transform().place({0, 2, 3});
	entity.transform().setEulerRotation({-10, 15, 30});

	expectCameraMatchesOwner(camera, entity);
}

TEST(CameraComponent3DContractTest, LocalMovementRefreshesViewWithoutChangingProjection)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	const auto before = camera.viewMatrix();
	const auto projection = camera.projectionMatrix();
	entity.transform().move({3, 1, -2});

	expectCameraMatchesOwner(camera, entity);
	EXPECT_NE(camera.viewMatrix()[3][0], before[3][0]);
	expectCameraMatrixNear(camera.projectionMatrix(), projection);
}

TEST(CameraComponent3DContractTest, ParentMovementRefreshesWorldView)
{
	spk::Entity3D parent("Parent");
	spk::Entity3D entity("Camera", &parent);
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	const auto before = camera.viewMatrix();
	parent.transform().move({5, 0, 0});

	expectCameraMatchesOwner(camera, entity);
	EXPECT_NE(camera.viewMatrix()[3][0], before[3][0]);
}

TEST(CameraComponent3DContractTest, ParentRotationRefreshesWorldView)
{
	spk::Entity3D parent("Parent");
	spk::Entity3D entity("Camera", &parent);
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	entity.transform().place({0, 0, 3});
	const auto before = camera.viewMatrix();
	parent.transform().setEulerRotation({0, 90, 0});

	expectCameraMatchesOwner(camera, entity);
	EXPECT_NE(camera.viewMatrix()[0][2], before[0][2]);
}

TEST(CameraComponent3DContractTest, ReparentingRecomputesViewAgainstNewParent)
{
	spk::Entity3D first("First");
	spk::Entity3D second("Second");
	spk::Entity3D entity("Camera", &first);
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	first.transform().place({2, 0, 0});
	second.transform().place({-4, 0, 0});
	const auto before = camera.viewMatrix();
	entity.setParent(second);

	expectCameraMatchesOwner(camera, entity);
	EXPECT_NE(camera.viewMatrix()[3][0], before[3][0]);
}

TEST(CameraComponent3DContractTest, CameraOwnScaleDoesNotAffectView)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	entity.transform().place({1, 2, 3});
	const auto before = camera.viewMatrix();
	entity.transform().rescale({2, 3, 4});

	expectCameraMatrixNear(camera.viewMatrix(), before);
}

TEST(CameraComponent3DContractTest, ProjectionChangeRefreshesViewProjectionMatrix)
{
	spk::Entity3D entity("Camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	entity.transform().place({1, 2, 3});
	const auto before = camera.viewProjectionMatrix();
	camera.setOrthographic(-3, 3, -2, 2, 0.1f, 40.0f);

	const auto expected = camera.projectionMatrix() * camera.viewMatrix();
	expectCameraMatrixNear(camera.viewProjectionMatrix(), expected);
	EXPECT_NE(camera.viewProjectionMatrix()[0][0], before[0][0]);
}

TEST(CameraComponent3DContractTest, DetachedCameraRejectsViewButAllowsProjection)
{
	spk::CameraComponent3D camera;
	camera.setPerspective(0.85f, 1.2f, 0.1f, 70.0f);

	EXPECT_EQ(camera.owner(), nullptr);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Perspective);
	EXPECT_THROW(static_cast<void>(camera.viewMatrix()), std::logic_error);
	EXPECT_THROW(static_cast<void>(camera.viewProjectionMatrix()), std::logic_error);
}

TEST(CameraComponent3DContractTest, CameraRejectsAnOwnerThatIsNotThreeDimensional)
{
	spk::Entity entity("2D owner");
	spk::CameraComponent3D camera;

	EXPECT_THROW(camera.attach(&entity), std::invalid_argument);
	EXPECT_EQ(camera.owner(), nullptr);
}
