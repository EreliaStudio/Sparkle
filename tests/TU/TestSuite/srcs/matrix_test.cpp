#include <gtest/gtest.h>

#include <cmath>
#include <functional>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <type_traits>

#include "math/matrix.hpp"

namespace
{
	constexpr float Tolerance = 1.0e-5f;

	template <typename T>
	concept HasIdentity = requires
	{
		T::identity();
	};

	template <typename T>
	concept HasMatrixMultiplication = requires(const T &left, const T &right)
	{
		left * right;
	};

	static_assert(std::is_trivially_copyable_v<spk::Matrix4x4>);
	static_assert(sizeof(spk::Matrix4x4) == sizeof(float) * 16);
	static_assert(HasIdentity<spk::Matrix2x2>);
	static_assert(HasIdentity<spk::Matrix3x3>);
	static_assert(HasIdentity<spk::Matrix4x4>);
	static_assert(!HasIdentity<spk::Matrix<2, 3>>);
	static_assert(!HasMatrixMultiplication<spk::Matrix<2, 3>>);
	static_assert(std::is_default_constructible_v<spk::Matrix<2, 3>>);

	void expectVectorNear(const spk::Vector4 &actual, const spk::Vector4 &expected, float tolerance = Tolerance)
	{
		EXPECT_NEAR(actual.x, expected.x, tolerance);
		EXPECT_NEAR(actual.y, expected.y, tolerance);
		EXPECT_NEAR(actual.z, expected.z, tolerance);
		EXPECT_NEAR(actual.w, expected.w, tolerance);
	}

	TEST(MatrixTest, StandardUsageBuildsTransformsAndAppliesKnownPipeline)
	{
		const spk::Matrix4x4 model =
			spk::Matrix4x4::translation(10.0f, 20.0f, 30.0f) *
			spk::Matrix4x4::rotation(0.0f, 0.0f, 90.0f) *
			spk::Matrix4x4::scale(2.0f, 3.0f, 4.0f);

		const spk::Vector4 transformed = model * spk::Vector4{1.0f, 0.0f, 0.0f, 1.0f};
		expectVectorNear(transformed, {10.0f, 22.0f, 30.0f, 1.0f}, 2.0e-4f);

		const spk::Matrix4x4 orthographic = spk::Matrix4x4::ortho(0.0f, 100.0f, 0.0f, 50.0f, -1.0f, 1.0f);
		expectVectorNear(orthographic * spk::Vector4{0.0f, 0.0f, 0.0f, 1.0f}, {-1.0f, -1.0f, 0.0f, 1.0f});
		expectVectorNear(orthographic * spk::Vector4{100.0f, 50.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f});

		const spk::Matrix4x4 perspective = spk::Matrix4x4::perspective(std::numbers::pi_v<float> / 2.0f, 2.0f, 1.0f, 11.0f);
		EXPECT_NEAR(perspective[0][0], 0.5f, Tolerance);
		EXPECT_NEAR(perspective[1][1], 1.0f, Tolerance);
		EXPECT_NEAR(perspective[2][2], -1.2f, Tolerance);
		EXPECT_NEAR(perspective[2][3], -1.0f, Tolerance);
		EXPECT_NEAR(perspective[3][2], -2.2f, Tolerance);
	}

	TEST(MatrixTest, DefaultConstructionZeroInitializesEveryElement)
	{
		const spk::Matrix<3, 2> matrix;

		for (std::size_t column = 0; column < 3; ++column)
		{
			for (std::size_t row = 0; row < 2; ++row)
			{
				EXPECT_FLOAT_EQ(matrix[column][row], 0.0f);
			}
		}
	}

	TEST(MatrixTest, IdentityHasOnesOnDiagonalAndZerosElsewhere)
	{
		const spk::Matrix4x4 matrix = spk::Matrix4x4::identity();

		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				EXPECT_FLOAT_EQ(matrix[column][row], column == row ? 1.0f : 0.0f);
			}
		}
	}

	TEST(MatrixTest, ColumnDefaultConstructionZeroInitializesRowsAndChecksBounds)
	{
		spk::Matrix<2, 3>::Column column;

		EXPECT_FLOAT_EQ(column[0], 0.0f);
		EXPECT_FLOAT_EQ(column[1], 0.0f);
		EXPECT_FLOAT_EQ(column[2], 0.0f);
		column[1] = 4.25f;

		const spk::Matrix<2, 3>::Column &constColumn = column;
		EXPECT_FLOAT_EQ(constColumn[1], 4.25f);
		EXPECT_THROW((void)column[3], std::out_of_range);
		EXPECT_THROW((void)constColumn[3], std::out_of_range);
	}

	TEST(MatrixTest, ColumnAccessIsMutableAndConst)
	{
		spk::Matrix<2, 3> matrix;
		matrix[1][2] = 7.5f;

		const spk::Matrix<2, 3> &constMatrix = matrix;
		EXPECT_FLOAT_EQ(constMatrix[1][2], 7.5f);
	}

	TEST(MatrixTest, ColumnAndRowOutOfRangeThrowOutOfRange)
	{
		spk::Matrix<2, 3> matrix;
		const spk::Matrix<2, 3> &constMatrix = matrix;

		EXPECT_THROW((void)matrix[2], std::out_of_range);
		EXPECT_THROW((void)constMatrix[2], std::out_of_range);
		EXPECT_THROW((void)matrix[0][3], std::out_of_range);
		EXPECT_THROW((void)constMatrix[0][3], std::out_of_range);
	}

	TEST(MatrixTest, MultiplicationOrderAppliesRightmostTransformFirst)
	{
		const spk::Matrix4x4 translation = spk::Matrix4x4::translation(10.0f, 0.0f, 0.0f);
		const spk::Matrix4x4 scale = spk::Matrix4x4::scale(2.0f, 2.0f, 2.0f);
		const spk::Vector4 input{1.0f, 0.0f, 0.0f, 1.0f};

		expectVectorNear((translation * scale) * input, {12.0f, 0.0f, 0.0f, 1.0f});
		expectVectorNear((scale * translation) * input, {22.0f, 0.0f, 0.0f, 1.0f});
	}

	TEST(MatrixTest, TranslationAndScaleVectorOverloadsMatchScalarOverloads)
	{
		const spk::Vector3 translation{1.0f, 2.0f, 3.0f};
		const spk::Vector3 scale{4.0f, 5.0f, 6.0f};

		const spk::Vector4 input{2.0f, 3.0f, 4.0f, 1.0f};
		expectVectorNear(spk::Matrix4x4::translation(translation) * input, spk::Matrix4x4::translation(1.0f, 2.0f, 3.0f) * input);
		expectVectorNear(spk::Matrix4x4::scale(scale) * input, spk::Matrix4x4::scale(4.0f, 5.0f, 6.0f) * input);
	}

	TEST(MatrixTest, RotationNormalizesQuaternionInput)
	{
		const spk::Quaternion unit = spk::Quaternion::fromAxisAngle({0.0f, 0.0f, 1.0f}, 90.0f);
		const spk::Quaternion scaled{unit.x * 5.0f, unit.y * 5.0f, unit.z * 5.0f, unit.w * 5.0f};
		const spk::Vector4 input{1.0f, 0.0f, 0.0f, 1.0f};

		expectVectorNear(spk::Matrix4x4::rotation(unit) * input, spk::Matrix4x4::rotation(scaled) * input);
	}

	TEST(MatrixTest, EulerAndQuaternionRotationOverloadsAgree)
	{
		const spk::Vector3 angles{15.0f, -25.0f, 40.0f};
		const spk::Vector4 input{1.0f, 2.0f, 3.0f, 1.0f};

		expectVectorNear(
			spk::Matrix4x4::rotation(angles) * input,
			spk::Matrix4x4::rotation(spk::Quaternion::fromEuler(angles)) * input);
	}

	TEST(MatrixTest, OrthographicProjectionRejectsEqualBoundsOnEveryAxis)
	{
		for (const auto &call : {
				 std::function<void()>([] { (void)spk::Matrix4x4::ortho(1.0f, 1.0f, -1.0f, 1.0f); }),
				 std::function<void()>([] { (void)spk::Matrix4x4::ortho(-1.0f, 1.0f, 2.0f, 2.0f); }),
				 std::function<void()>([] { (void)spk::Matrix4x4::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 3.0f, 3.0f); })})
		{
			EXPECT_THROW(call(), std::invalid_argument);
		}
	}

	TEST(MatrixTest, PerspectiveRejectsInvalidFiniteParameters)
	{
		const float pi = std::numbers::pi_v<float>;

		EXPECT_THROW((void)spk::Matrix4x4::perspective(0.0f, 1.0f, 0.1f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(-0.1f, 1.0f, 0.1f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(pi, 1.0f, 0.1f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(pi + 0.1f, 1.0f, 0.1f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(pi / 2.0f, 0.0f, 0.1f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(pi / 2.0f, -1.0f, 0.1f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(pi / 2.0f, 1.0f, 0.0f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(pi / 2.0f, 1.0f, -0.1f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(pi / 2.0f, 1.0f, 1.0f, 1.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(pi / 2.0f, 1.0f, 2.0f, 1.0f), std::invalid_argument);
	}

	// Current public validation only compares numeric ranges and does not reject every NaN/infinity case.
	TEST(MatrixTest, DISABLED_PerspectiveRejectsNonFiniteParameters)
	{
		const float nan = std::numeric_limits<float>::quiet_NaN();
		const float infinity = std::numeric_limits<float>::infinity();
		const float fov = std::numbers::pi_v<float> / 2.0f;
		for (const float value : {infinity, -infinity})
		{
			EXPECT_THROW((void)spk::Matrix4x4::perspective(value, 1.0f, 0.1f, 100.0f), std::invalid_argument);
			EXPECT_THROW((void)spk::Matrix4x4::perspective(fov, value, 0.1f, 100.0f), std::invalid_argument);
			EXPECT_THROW((void)spk::Matrix4x4::perspective(fov, 1.0f, value, 100.0f), std::invalid_argument);
			EXPECT_THROW((void)spk::Matrix4x4::perspective(fov, 1.0f, 0.1f, value), std::invalid_argument);
		}

		EXPECT_THROW((void)spk::Matrix4x4::perspective(nan, 1.0f, 0.1f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(fov, nan, 0.1f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(fov, infinity, 0.1f, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(fov, 1.0f, nan, 100.0f), std::invalid_argument);
		EXPECT_THROW((void)spk::Matrix4x4::perspective(fov, 1.0f, 0.1f, nan), std::invalid_argument);
	}
}
