#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <utility>

#include "math/camera_3d.hpp"

namespace
{
	constexpr float Tolerance = 0.0002f;

	void expectMatrix(const spk::Matrix4x4 &actual, const spk::Matrix4x4 &expected)
	{
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				EXPECT_NEAR(actual[column][row], expected[column][row], Tolerance);
			}
		}
	}

	void expectVector(const spk::Vector4 &actual, const spk::Vector4 &expected)
	{
		EXPECT_NEAR(actual.x, expected.x, Tolerance);
		EXPECT_NEAR(actual.y, expected.y, Tolerance);
		EXPECT_NEAR(actual.z, expected.z, Tolerance);
		EXPECT_NEAR(actual.w, expected.w, Tolerance);
	}
}

TEST(Camera3DTest, DefaultsToIdentityPoseAndPerspectiveProjection)
{
	const spk::Camera3D camera;
	EXPECT_EQ(camera.position(), spk::Vector3(0, 0, 0));
	EXPECT_EQ(camera.rotation(), spk::Quaternion::identity());
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Perspective);
	expectMatrix(camera.viewMatrix(), spk::Matrix4x4::identity());
	expectMatrix(camera.viewProjectionMatrix(), camera.projectionMatrix());
	EXPECT_GT(camera.projectionMatrix()[0][0], 0.0f);
	EXPECT_LT(camera.projectionMatrix()[2][3], 0.0f);
}

TEST(Camera3DTest, ViewMatrixIsInverseOfTranslationAndRotation)
{
	spk::Camera3D camera;
	camera.setPosition({1, 2, 5});
	camera.setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	const auto expected = spk::Matrix4x4::rotation(0, 0, -90) *
						  spk::Matrix4x4::translation(-1, -2, -5);
	expectMatrix(camera.viewMatrix(), expected);
	expectVector(camera.viewMatrix() * spk::Vector4{1, 2, 5, 1}, {0, 0, 0, 1});
	expectVector(camera.viewMatrix() * spk::Vector4{2, 2, 5, 1}, {0, -1, 0, 1});
}

TEST(Camera3DTest, ViewCacheIsInvalidatedByPositionAndRotation)
{
	spk::Camera3D camera;
	(void)camera.viewMatrix();
	(void)camera.viewProjectionMatrix();
	camera.setPosition({0, 0, 5});
	expectMatrix(camera.viewMatrix(), spk::Matrix4x4::translation(0, 0, -5));
	expectMatrix(camera.viewProjectionMatrix(), camera.projectionMatrix() * camera.viewMatrix());
	camera.setRotation(spk::Quaternion::fromEuler({0, 90, 0}));
	const auto rotated = spk::Matrix4x4::rotation(0, -90, 0) * spk::Matrix4x4::translation(0, 0, -5);
	expectMatrix(camera.viewMatrix(), rotated);
	expectMatrix(camera.viewProjectionMatrix(), camera.projectionMatrix() * camera.viewMatrix());
}

TEST(Camera3DTest, OrthographicProjectionMatchesKnownCoefficientValues)
{
	spk::Camera3D camera;
	camera.setOrthographic(-4, 6, -3, 5, 1, 11);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Orthographic);
	auto expected = spk::Matrix4x4::identity();
	expected[0][0] = 0.2f;
	expected[1][1] = 0.25f;
	expected[2][2] = -0.2f;
	expected[3][0] = -0.2f;
	expected[3][1] = -0.25f;
	expected[3][2] = -1.2f;
	expectMatrix(camera.projectionMatrix(), expected);
	expectVector(camera.viewProjectionMatrix() * spk::Vector4{1, 1, -6, 1}, {0, 0, 0, 1});
}

TEST(Camera3DTest, OrthographicProjectionPreservesSizeWithDistance)
{
	spk::Camera3D camera;
	camera.setOrthographic(-2, 2, -2, 2, 0.1f, 100.0f);
	const auto nearPoint = camera.viewProjectionMatrix() * spk::Vector4{1, 0.5f, -2, 1};
	const auto farPoint = camera.viewProjectionMatrix() * spk::Vector4{1, 0.5f, -50, 1};
	EXPECT_NEAR(nearPoint.x, farPoint.x, Tolerance);
	EXPECT_NEAR(nearPoint.y, farPoint.y, Tolerance);
	EXPECT_NEAR(nearPoint.w, 1.0f, Tolerance);
	EXPECT_NEAR(farPoint.w, 1.0f, Tolerance);
	EXPECT_NE(nearPoint.z, farPoint.z);
}

TEST(Camera3DTest, PerspectiveProjectionMatchesKnownCoefficientValues)
{
	spk::Camera3D camera;
	camera.setPerspective(std::numbers::pi_v<float> / 2.0f, 2.0f, 1.0f, 11.0f);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Perspective);
	spk::Matrix4x4 expected{};
	expected[0][0] = 0.5f;
	expected[1][1] = 1.0f;
	expected[2][2] = -1.2f;
	expected[2][3] = -1.0f;
	expected[3][2] = -2.2f;
	expectMatrix(camera.projectionMatrix(), expected);
	expectVector(camera.viewProjectionMatrix() * spk::Vector4{2, 1, -4, 1}, {1, 1, 2.6f, 4});
}

TEST(Camera3DTest, ChangingProjectionModeRestoresEachSavedProjection)
{
	spk::Camera3D camera;
	camera.setPerspective(1.2f, 1.5f, 0.2f, 80);
	const auto perspective = camera.projectionMatrix();
	camera.setOrthographic(-2, 4, -3, 3, 0.1f, 20);
	const auto orthographic = camera.projectionMatrix();
	(void)camera.viewProjectionMatrix();
	camera.setProjectionMode(spk::Camera3D::ProjectionMode::Perspective);
	expectMatrix(camera.projectionMatrix(), perspective);
	expectMatrix(camera.viewProjectionMatrix(), perspective * camera.viewMatrix());
	camera.setProjectionMode(spk::Camera3D::ProjectionMode::Orthographic);
	expectMatrix(camera.projectionMatrix(), orthographic);
	expectMatrix(camera.viewProjectionMatrix(), orthographic * camera.viewMatrix());
}

TEST(Camera3DTest, SwitchingProjectionDoesNotChangePose)
{
	spk::Camera3D camera;
	camera.setPosition({2, -3, 7});
	camera.setRotation(spk::Quaternion::fromEuler({20, 30, -15}));
	const auto view = camera.viewMatrix();
	camera.setOrthographic(-5, 5, -5, 5, 0.1f, 50);
	expectMatrix(camera.viewMatrix(), view);
	camera.setPerspective(1.2f, 1.0f, 0.1f, 50);
	expectMatrix(camera.viewMatrix(), view);
}

TEST(Camera3DTest, LookAtTargetsOriginFromObliquePosition)
{
	spk::Camera3D camera;
	camera.setPosition({3, 2, 6});
	camera.lookAt({0, 0, 0});
	const auto target = camera.viewMatrix() * spk::Vector4{0, 0, 0, 1};
	EXPECT_NEAR(target.x, 0.0f, Tolerance);
	EXPECT_NEAR(target.y, 0.0f, Tolerance);
	EXPECT_NEAR(target.z, -7.0f, Tolerance);
	EXPECT_NEAR(target.w, 1.0f, Tolerance);
	EXPECT_NEAR(camera.rotation().dot(camera.rotation()), 1.0f, Tolerance);
}

TEST(Camera3DTest, LookAtAlongNegativeZProducesIdentityOrientation)
{
	spk::Camera3D camera;
	camera.setPosition({0, 0, 5});
	camera.lookAt({0, 0, 0});
	expectMatrix(camera.viewMatrix(), spk::Matrix4x4::translation(0, 0, -5));
}

TEST(Camera3DTest, InvalidOrthographicParametersPreservePreviousProjection)
{
	spk::Camera3D camera;
	camera.setOrthographic(-2, 2, -2, 2, 0.1f, 20);
	const auto previous = camera.projectionMatrix();
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float infinity = std::numeric_limits<float>::infinity();
	EXPECT_THROW(camera.setOrthographic(1, 1, -2, 2, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setOrthographic(2, -2, -2, 2, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setOrthographic(-2, 2, 3, -3, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setOrthographic(-2, 2, -2, 2, 20, 20), std::invalid_argument);
	EXPECT_THROW(camera.setOrthographic(nan, 2, -2, 2, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setOrthographic(-2, infinity, -2, 2, 0.1f, 20), std::invalid_argument);
	expectMatrix(camera.projectionMatrix(), previous);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Orthographic);
}

TEST(Camera3DTest, InvalidPerspectiveParametersPreservePreviousProjection)
{
	spk::Camera3D camera;
	camera.setOrthographic(-2, 2, -2, 2, 0.1f, 20);
	const auto previous = camera.projectionMatrix();
	EXPECT_THROW(camera.setPerspective(0, 1, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(3.2f, 1, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(1, 0, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(1, 1, 0, 20), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(1, 1, 20, 1), std::invalid_argument);
	expectMatrix(camera.projectionMatrix(), previous);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Orthographic);
	EXPECT_THROW(camera.setProjectionMode(static_cast<spk::Camera3D::ProjectionMode>(255)), std::invalid_argument);
}

TEST(Camera3DTest, InvalidPoseAndLookAtDoNotChangeCachedView)
{
	spk::Camera3D camera;
	camera.setPosition({0, 0, 5});
	const auto before = camera.viewMatrix();
	const float invalid = std::numeric_limits<float>::infinity();
	EXPECT_THROW(camera.setPosition({invalid, 0, 0}), std::invalid_argument);
	EXPECT_THROW(camera.setRotation({0, 0, 0, 0}), std::domain_error);
	EXPECT_THROW(camera.setRotation({invalid, 0, 0, 1}), std::invalid_argument);
	EXPECT_THROW(camera.lookAt({0, 0, 5}), std::exception);
	EXPECT_THROW(camera.lookAt({0, invalid, 0}), std::invalid_argument);
	expectMatrix(camera.viewMatrix(), before);
}

TEST(Camera3DTest, CopyAndMoveConstructorsRebindCachedMatrixGenerators)
{
	spk::Camera3D original;
	original.setPosition({1, 2, 5});
	original.setOrthographic(-3, 3, -3, 3, 0.1f, 30);
	(void)original.viewMatrix();
	(void)original.viewProjectionMatrix();
	spk::Camera3D copy(original);
	spk::Camera3D moved(std::move(copy));
	original.setPosition({50, 50, 50});
	expectMatrix(moved.viewMatrix(), spk::Matrix4x4::translation(-1, -2, -5));
	EXPECT_EQ(moved.projectionMode(), spk::Camera3D::ProjectionMode::Orthographic);
	expectMatrix(moved.viewProjectionMatrix(), moved.projectionMatrix() * moved.viewMatrix());
}

TEST(Camera3DTest, CopyAndMoveAssignmentsKeepIndependentCaches)
{
	spk::Camera3D source;
	source.setPosition({1, 2, 3});
	source.setOrthographic(-2, 2, -4, 4, 0.1f, 30);
	spk::Camera3D target;
	(void)target.viewMatrix();
	(void)target.viewProjectionMatrix();
	target = source;
	expectMatrix(target.viewMatrix(), spk::Matrix4x4::translation(-1, -2, -3));
	source.setPosition({4, 5, 6});
	target = std::move(source);
	expectMatrix(target.viewMatrix(), spk::Matrix4x4::translation(-4, -5, -6));
	expectMatrix(target.viewProjectionMatrix(), target.projectionMatrix() * target.viewMatrix());
}

TEST(Camera3DTest, ThreeQuarterOrthographicCameraProjectsKnownWorldDirections)
{
	spk::Camera3D camera;
	camera.setPosition({5, 5, 5});
	camera.lookAt({0, 0, 0});
	camera.setOrthographic(-3, 3, -3, 3, 0.1f, 20);
	ASSERT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Orthographic);

	const auto view = camera.viewMatrix();
	const float diagonal = std::sqrt(75.0f);
	const auto center = view * spk::Vector4{0, 0, 0, 1};
	expectVector(center, {0, 0, -diagonal, 1});
	expectVector(view * spk::Vector4{5, 5, 5, 1}, {0, 0, 0, 1});

	const auto x = view * spk::Vector4{1, 0, 0, 0};
	const auto y = view * spk::Vector4{0, 1, 0, 0};
	const auto z = view * spk::Vector4{0, 0, 1, 0};
	const float inverseSqrt2 = 1.0f / std::sqrt(2.0f);
	const float inverseSqrt6 = 1.0f / std::sqrt(6.0f);
	EXPECT_NEAR(x.x, inverseSqrt2, Tolerance);
	EXPECT_NEAR(y.x, 0.0f, Tolerance);
	EXPECT_NEAR(z.x, -inverseSqrt2, Tolerance);
	EXPECT_NEAR(x.y, -inverseSqrt6, Tolerance);
	EXPECT_NEAR(y.y, 2.0f * inverseSqrt6, Tolerance);
	EXPECT_NEAR(z.y, -inverseSqrt6, Tolerance);
	EXPECT_NEAR(x.z, 1.0f / std::sqrt(3.0f), Tolerance);
	EXPECT_NEAR(y.z, 1.0f / std::sqrt(3.0f), Tolerance);
	EXPECT_NEAR(z.z, 1.0f / std::sqrt(3.0f), Tolerance);
}

TEST(Camera3DTest, OrthographicProjectionMapsBoundsAndClippingPlanesToUnitCube)
{
	spk::Camera3D camera;
	camera.setOrthographic(-3, 3, -3, 3, 0.1f, 20);
	const auto projection = camera.viewProjectionMatrix();
	expectVector(projection * spk::Vector4{-3, -3, -0.1f, 1}, {-1, -1, -1, 1});
	expectVector(projection * spk::Vector4{3, 3, -20, 1}, {1, 1, 1, 1});
	expectVector(projection * spk::Vector4{0, 0, -10.05f, 1}, {0, 0, 0, 1});
}

TEST(Camera3DTest, PerspectiveProjectionChangesApparentSizeWithDepth)
{
	spk::Camera3D camera;
	camera.setPerspective(std::numbers::pi_v<float> / 2, 1, 1, 11);
	const auto projection = camera.viewProjectionMatrix();
	const auto nearPoint = projection * spk::Vector4{1, 1, -1, 1};
	const auto farPoint = projection * spk::Vector4{1, 1, -11, 1};
	expectVector(nearPoint, {1, 1, -1, 1});
	expectVector(farPoint, {1, 1, 11, 11});
	EXPECT_NEAR(nearPoint.x / nearPoint.w, 1.0f, Tolerance);
	EXPECT_NEAR(farPoint.x / farPoint.w, 1.0f / 11.0f, Tolerance);
}

TEST(Camera3DTest, ThreeQuarterLookAtRemainsConsistentWithCustomTarget)
{
	spk::Camera3D camera;
	camera.setPosition({8, 7, 6});
	const spk::Vector3 target{1, 2, 3};
	camera.lookAt(target);
	const auto centered = camera.viewMatrix() * spk::Vector4{target.x, target.y, target.z, 1};
	const float distance = std::sqrt(7.0f * 7 + 5.0f * 5 + 3.0f * 3);
	expectVector(centered, {0, 0, -distance, 1});
	EXPECT_NEAR(camera.rotation().dot(camera.rotation()), 1.0f, Tolerance);

	camera.lookAt(target, {0, 0, 1});
	const auto customUp = camera.viewMatrix() * spk::Vector4{target.x, target.y, target.z, 1};
	expectVector(customUp, {0, 0, -distance, 1});
}

TEST(Camera3DTest, LookAtParallelUpUsesFallbackWithoutChangingTarget)
{
	spk::Camera3D camera;
	camera.setPosition({0, 5, 0});
	camera.lookAt({0, 0, 0}, {0, 1, 0});
	expectVector(camera.viewMatrix() * spk::Vector4{0, 0, 0, 1}, {0, 0, -5, 1});
	EXPECT_NEAR(camera.rotation().dot(camera.rotation()), 1.0f, Tolerance);
}

TEST(Camera3DTest, InvalidProjectionParametersLeaveModeMatricesAndViewUnchanged)
{
	spk::Camera3D camera;
	camera.setPosition({5, 5, 5});
	camera.lookAt({0, 0, 0});
	camera.setOrthographic(-3, 3, -3, 3, 0.1f, 20);
	const auto view = camera.viewMatrix();
	const auto projection = camera.projectionMatrix();
	const auto combined = camera.viewProjectionMatrix();
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float infinity = std::numeric_limits<float>::infinity();
	EXPECT_THROW(camera.setOrthographic(-3, 3, nan, 3, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setOrthographic(-3, 3, -3, 3, nan, 20), std::invalid_argument);
	EXPECT_THROW(camera.setOrthographic(-3, 3, -3, infinity, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setOrthographic(-3, 3, -3, 3, 0.1f, infinity), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(nan, 1, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(1, infinity, 0.1f, 20), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(1, 1, nan, 20), std::invalid_argument);
	EXPECT_THROW(camera.setPerspective(1, 1, 0.1f, infinity), std::invalid_argument);
	expectMatrix(camera.viewMatrix(), view);
	expectMatrix(camera.projectionMatrix(), projection);
	expectMatrix(camera.viewProjectionMatrix(), combined);
	EXPECT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Orthographic);
}

TEST(Camera3DTest, ExplicitProjectionModeSwitchPreservesSeparateFrustums)
{
	spk::Camera3D camera;
	camera.setOrthographic(-3, 3, -3, 3, 0.1f, 20);
	const auto orthographic = camera.projectionMatrix();
	camera.setPerspective(1.1f, 1.75f, 0.25f, 45);
	const auto perspective = camera.projectionMatrix();
	camera.setProjectionMode(spk::Camera3D::ProjectionMode::Orthographic);
	expectMatrix(camera.projectionMatrix(), orthographic);
	camera.setProjectionMode(spk::Camera3D::ProjectionMode::Perspective);
	expectMatrix(camera.projectionMatrix(), perspective);
	camera.setProjectionMode(spk::Camera3D::ProjectionMode::Orthographic);
	expectMatrix(camera.viewProjectionMatrix(), orthographic * camera.viewMatrix());
}

TEST(Camera3DTest, ScaledQuaternionKeepsCameraOrientationNormalized)
{
	spk::Camera3D camera;
	const auto rotation = spk::Quaternion::fromEuler({17, 29, -41});
	camera.setRotation({rotation.x * 5, rotation.y * 5, rotation.z * 5, rotation.w * 5});
	EXPECT_NEAR(std::abs(camera.rotation().dot(rotation)), 1.0f, Tolerance);
	expectMatrix(camera.viewMatrix(), spk::Matrix4x4::rotation(rotation.conjugated()));
}

TEST(Camera3DTest, SelfAssignmentAndCopiesRetainIndependentThreeQuarterProjection)
{
	spk::Camera3D camera;
	camera.setPosition({5, 5, 5});
	camera.lookAt({0, 0, 0});
	camera.setOrthographic(-3, 3, -3, 3, 0.1f, 20);
	const auto before = camera.viewProjectionMatrix();
	camera = camera;
	expectMatrix(camera.viewProjectionMatrix(), before);
	camera = std::move(camera);
	expectMatrix(camera.viewProjectionMatrix(), before);

	spk::Camera3D copy(camera);
	copy.setPosition({0, 0, 8});
	copy.lookAt({0, 0, 0});
	expectMatrix(camera.viewProjectionMatrix(), before);
	EXPECT_NEAR((copy.viewMatrix() * spk::Vector4{0, 0, 0, 1}).z, -8.0f, Tolerance);
}
