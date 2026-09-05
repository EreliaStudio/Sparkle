#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

#include "math/matrix.hpp"
#include "math/quaternion.hpp"

namespace
{
	constexpr float Tolerance = 1.0e-4f;

	void expectQuaternionEquivalent(const spk::Quaternion &actual, const spk::Quaternion &expected, float tolerance = Tolerance)
	{
		const spk::Quaternion a = actual.normalized();
		const spk::Quaternion e = expected.normalized();
		const float dot = std::fabs(a.dot(e));
		EXPECT_NEAR(dot, 1.0f, tolerance);
	}

	void expectVectorNear(const spk::Vector4 &actual, const spk::Vector4 &expected, float tolerance = Tolerance)
	{
		EXPECT_NEAR(actual.x, expected.x, tolerance);
		EXPECT_NEAR(actual.y, expected.y, tolerance);
		EXPECT_NEAR(actual.z, expected.z, tolerance);
		EXPECT_NEAR(actual.w, expected.w, tolerance);
	}

	TEST(QuaternionTest, StandardUsageCreatesComposesInvertsRotatesConvertsAndInterpolates)
	{
		const spk::Quaternion axisAngle = spk::Quaternion::fromAxisAngle({0.0f, 0.0f, 1.0f}, 90.0f);
		const spk::Quaternion euler = spk::Quaternion::fromEuler({0.0f, 0.0f, 90.0f});
		expectQuaternionEquivalent(axisAngle, euler);

		const spk::Quaternion composed = spk::Quaternion::identity() * axisAngle;
		expectQuaternionEquivalent(composed, axisAngle);

		const spk::Quaternion inverse = axisAngle.inversed();
		expectQuaternionEquivalent(axisAngle * inverse, spk::Quaternion::identity());

		const spk::Vector4 rotated = spk::Matrix4x4::rotation(axisAngle) * spk::Vector4{1.0f, 0.0f, 0.0f, 1.0f};
		expectVectorNear(rotated, {0.0f, 1.0f, 0.0f, 1.0f});

		const spk::Vector3 convertedEuler = euler.toEuler();
		expectQuaternionEquivalent(spk::Quaternion::fromEuler(convertedEuler), euler);

		const spk::Quaternion halfway = spk::Quaternion::slerp(
			spk::Quaternion::identity(),
			spk::Quaternion::fromAxisAngle({0.0f, 0.0f, 1.0f}, 120.0f),
			0.5f);
		const spk::Vector4 halfwayRotated = spk::Matrix4x4::rotation(halfway) * spk::Vector4{1.0f, 0.0f, 0.0f, 1.0f};
		expectVectorNear(halfwayRotated, {0.5f, 0.8660254f, 0.0f, 1.0f}, 5.0e-4f);
	}

	TEST(QuaternionTest, IdentityAndDefaultConstructionRepresentTheSameRotation)
	{
		EXPECT_EQ(spk::Quaternion{}, spk::Quaternion::identity());
		expectQuaternionEquivalent(spk::Quaternion::identity() * spk::Quaternion::identity(), spk::Quaternion::identity());
	}

	TEST(QuaternionTest, ConjugateNegatesVectorPartOnly)
	{
		const spk::Quaternion value{1.0f, -2.0f, 3.0f, -4.0f};

		EXPECT_EQ(value.conjugated(), (spk::Quaternion{-1.0f, 2.0f, -3.0f, -4.0f}));
	}

	TEST(QuaternionTest, InverseHandlesNonUnitQuaternion)
	{
		const spk::Quaternion value{1.0f, 2.0f, 3.0f, 4.0f};
		const spk::Quaternion identity = value * value.inversed();

		expectQuaternionEquivalent(identity, spk::Quaternion::identity());
	}

	TEST(QuaternionTest, InvertingZeroQuaternionThrowsRuntimeError)
	{
		try
		{
			(void)spk::Quaternion{0.0f, 0.0f, 0.0f, 0.0f}.inversed();
			FAIL() << "Expected std::runtime_error";
		}
		catch (const std::runtime_error &exception)
		{
			EXPECT_STREQ(exception.what(), "Can't inverse a null quaternion");
		}
	}

	TEST(QuaternionTest, AxisAngleNormalizesTheInputAxis)
	{
		const spk::Quaternion unitAxis = spk::Quaternion::fromAxisAngle({0.0f, 0.0f, 1.0f}, 60.0f);
		const spk::Quaternion scaledAxis = spk::Quaternion::fromAxisAngle({0.0f, 0.0f, 10.0f}, 60.0f);

		expectQuaternionEquivalent(unitAxis, scaledAxis);
	}

	TEST(QuaternionTest, NormalizedAcceptsNonUnitInput)
	{
		const spk::Quaternion normalized = spk::Quaternion{0.0f, 0.0f, 0.0f, 7.0f}.normalized();

		EXPECT_NEAR(normalized.x, 0.0f, Tolerance);
		EXPECT_NEAR(normalized.y, 0.0f, Tolerance);
		EXPECT_NEAR(normalized.z, 0.0f, Tolerance);
		EXPECT_NEAR(normalized.w, 1.0f, Tolerance);
	}

	TEST(QuaternionTest, NormalizeZeroQuaternionThrowsDomainError)
	{
		EXPECT_THROW(((void)spk::Quaternion(0.0f, 0.0f, 0.0f, 0.0f).normalized()), std::domain_error);
	}

	TEST(QuaternionTest, SlerpTreatsOppositeQuaternionSignsAsTheSameRotation)
	{
		const spk::Quaternion identity = spk::Quaternion::identity();
		const spk::Quaternion oppositeSign{0.0f, 0.0f, 0.0f, -1.0f};

		expectQuaternionEquivalent(spk::Quaternion::slerp(identity, oppositeSign, 0.5f), identity);
	}

	TEST(QuaternionTest, SlerpHandlesNearlyIdenticalOrientations)
	{
		const spk::Quaternion from = spk::Quaternion::fromAxisAngle({0.0f, 1.0f, 0.0f}, 10.0f);
		const spk::Quaternion to = spk::Quaternion::fromAxisAngle({0.0f, 1.0f, 0.0f}, 10.001f);
		const spk::Quaternion midpoint = spk::Quaternion::slerp(from, to, 0.5f);

		expectQuaternionEquivalent(midpoint, spk::Quaternion::fromAxisAngle({0.0f, 1.0f, 0.0f}, 10.0005f), 5.0e-4f);
	}

	TEST(QuaternionTest, SlerpReturnsEndpointsAtZeroAndOne)
	{
		const spk::Quaternion from = spk::Quaternion::fromEuler({10.0f, 20.0f, 30.0f});
		const spk::Quaternion to = spk::Quaternion::fromEuler({-20.0f, 40.0f, 15.0f});

		expectQuaternionEquivalent(spk::Quaternion::slerp(from, to, 0.0f), from);
		expectQuaternionEquivalent(spk::Quaternion::slerp(from, to, 1.0f), to);
	}

	TEST(QuaternionTest, SlerpSupportsExtrapolationOutsideUnitInterval)
	{
		const spk::Quaternion from = spk::Quaternion::identity();
		const spk::Quaternion to = spk::Quaternion::fromAxisAngle({0.0f, 0.0f, 1.0f}, 90.0f);

		expectQuaternionEquivalent(
			spk::Quaternion::slerp(from, to, 1.5f),
			spk::Quaternion::fromAxisAngle({0.0f, 0.0f, 1.0f}, 135.0f),
			5.0e-4f);
		expectQuaternionEquivalent(
			spk::Quaternion::slerp(from, to, -0.5f),
			spk::Quaternion::fromAxisAngle({0.0f, 0.0f, 1.0f}, -45.0f),
			5.0e-4f);
	}

	TEST(QuaternionTest, EulerRoundTripPreservesRotationNearGimbalLock)
	{
		for (const float pitch : {89.9f, 90.0f, 90.1f, -89.9f, -90.0f, -90.1f})
		{
			const spk::Quaternion original = spk::Quaternion::fromEuler({15.0f, pitch, -25.0f});
			const spk::Quaternion roundTrip = spk::Quaternion::fromEuler(original.toEuler());
			expectQuaternionEquivalent(roundTrip, original, 2.0e-3f);
		}
	}

	TEST(QuaternionTest, LookAtForwardNegativeZProducesIdentityRotation)
	{
		const spk::Quaternion rotation = spk::Quaternion::lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f});

		expectQuaternionEquivalent(rotation, spk::Quaternion::identity());
	}

	TEST(QuaternionTest, LookAtRotatesNegativeZTowardTargetDirection)
	{
		const spk::Quaternion rotation = spk::Quaternion::lookAt({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
		const spk::Vector4 forward = spk::Matrix4x4::rotation(rotation) * spk::Vector4{0.0f, 0.0f, -1.0f, 0.0f};

		expectVectorNear(forward, {1.0f, 0.0f, 0.0f, 0.0f}, 5.0e-4f);
	}

	TEST(QuaternionTest, LookAtHandlesUpVectorCollinearWithForwardDirection)
	{
		const spk::Quaternion rotation = spk::Quaternion::lookAt(
			{0.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, 0.0f},
			{0.0f, 1.0f, 0.0f});

		const spk::Quaternion normalized = rotation.normalized();
		EXPECT_NEAR(normalized.dot(normalized), 1.0f, Tolerance);
	}

	TEST(QuaternionTest, LookAtWithCoincidentPointsRejectsDegenerateForwardVector)
	{
		EXPECT_THROW(
			(void)spk::Quaternion::lookAt({1.0f, 2.0f, 3.0f}, {1.0f, 2.0f, 3.0f}),
			std::domain_error);
	}
}
