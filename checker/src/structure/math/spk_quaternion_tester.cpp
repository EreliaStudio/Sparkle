#include "structure/math/spk_quaternion_tester.hpp"

#include <cmath>

TEST_F(QuaternionTest, EulerRoundTrip)
{
	spk::Vector3 euler(45.0f, -60.0f, 30.0f);
	spk::Quaternion q = spk::Quaternion::fromEuler(euler);
	spk::Vector3 restored = q.toEuler();

	EXPECT_NEAR(restored.x, euler.x, 1e-3f);
	EXPECT_NEAR(restored.y, euler.y, 1e-3f);
	EXPECT_NEAR(restored.z, euler.z, 1e-3f);
}

TEST_F(QuaternionTest, AxisAngleRotation)
{
	spk::Quaternion rot = spk::Quaternion::fromAxisAngle(spk::Vector3(0.0f, 0.0f, 1.0f), 90.0f);
	spk::Vector3 rotated = rot.rotate(spk::Vector3(1.0f, 0.0f, 0.0f));

	EXPECT_NEAR(rotated.x, 0.0f, 1e-4f);
	EXPECT_NEAR(rotated.y, 1.0f, 1e-4f);
	EXPECT_NEAR(rotated.z, 0.0f, 1e-4f);
}

TEST_F(QuaternionTest, LookAtMatchesForward)
{
	spk::Vector3 eye(0.0f, 0.0f, 0.0f);
	spk::Vector3 target(1.0f, 1.0f, 0.0f);
	spk::Vector3 up(0.0f, 1.0f, 0.0f);

	spk::Quaternion look = spk::Quaternion::lookAt(eye, target, up);
	spk::Vector3 rotated = look.rotate(spk::Vector3(0.0f, 0.0f, -1.0f)).normalize();
	spk::Vector3 forward = (target - eye).normalize();

	EXPECT_NEAR(rotated.x, forward.x, 1e-4f);
	EXPECT_NEAR(rotated.y, forward.y, 1e-4f);
	EXPECT_NEAR(rotated.z, forward.z, 1e-4f);
}

TEST_F(QuaternionTest, LookAtHandlesParallelUp)
{
	spk::Vector3 eye(0.0f, 0.0f, 0.0f);
	spk::Vector3 target(0.0f, 1.0f, 0.0f);
	spk::Vector3 up(0.0f, 1.0f, 0.0f);

	spk::Quaternion look = spk::Quaternion::lookAt(eye, target, up);
	spk::Vector3 rotated = look.rotate(spk::Vector3(0.0f, 0.0f, -1.0f)).normalize();
	spk::Vector3 forward = (target - eye).normalize();

	EXPECT_NEAR(rotated.x, forward.x, 1e-4f);
	EXPECT_NEAR(rotated.y, forward.y, 1e-4f);
	EXPECT_NEAR(rotated.z, forward.z, 1e-4f);
}
