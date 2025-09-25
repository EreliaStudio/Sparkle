#include "structure/math/spk_matrix_tester.hpp"

#include "structure/math/spk_vector3.hpp"

TEST_F(MatrixTest, MatrixConstructor)
{
	spk::Matrix4x4 defaultMatrix;

	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			if (i == j)
			{
				ASSERT_EQ(defaultMatrix[i][j], 1) << "Default matrix on [" << i << "][" << j << "] should be 1";
			}
			else
			{
				ASSERT_EQ(defaultMatrix[i][j], 0) << "Default matrix on [" << i << "][" << j << "] should be 0";
			}
		}
	}
}

TEST_F(MatrixTest, MatrixConstructorOrder)
{
	float data[16] = {2, 0, 0, 3, 0, 2, 0, 4, 0, 0, 2, 5, 0, 0, 0, 1};

	spk::Matrix4x4 customMatrix = spk::Matrix4x4(data);

	for (size_t i = 0; i < 4; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			ASSERT_EQ(customMatrix[i][j], data[i * 4 + j]) << "Matrix should be column major";
		}
	}
}

TEST_F(MatrixTest, MatrixOnVector3Application)
{
	spk::Matrix4x4 customMatrix = spk::Matrix4x4({2, 0, 0, 3, 0, 2, 0, 4, 0, 0, 2, 5, 0, 0, 0, 1});

	spk::Vector3 position = spk::Vector3(1, 2, 3);

	spk::Vector3 computedPosition = customMatrix * position;

	ASSERT_EQ(computedPosition, spk::Vector3(5, 8, 11)) << "Wrong matrix multiplication";
}

TEST_F(MatrixTest, MatrixOnVector2Application)
{
	spk::Matrix3x3 customMatrix = spk::Matrix3x3({2, 0, 3, 0, 2, 4, 0, 0, 1});

	spk::Vector2 position = spk::Vector2(1, 2);

	spk::Vector2 computedPosition = customMatrix * position;

	ASSERT_EQ(computedPosition, spk::Vector2(5, 8)) << "Wrong matrix multiplication";
}

TEST_F(MatrixTest, MatrixComparator)
{
	spk::Matrix4x4 matrix = spk::Matrix4x4({2, 0, 0, 3, 0, 2, 0, 4, 0, 0, 2, 5, 0, 0, 0, 1});

	spk::Matrix4x4 differentMatrix = spk::Matrix4x4({2, 0, 0, 3, 0, 2, 0, 5, 0, 0, 2, 5, 0, 0, 0, 1});

	spk::Matrix4x4 identicalMatrix = spk::Matrix4x4({2, 0, 0, 3, 0, 2, 0, 4, 0, 0, 2, 5, 0, 0, 0, 1});

	EXPECT_NE(matrix, differentMatrix);
	ASSERT_EQ(matrix, identicalMatrix);
}

TEST_F(MatrixTest, MatrixToString)
{
	spk::Matrix4x4 matrix = spk::Matrix4x4({2, 0, 0, 3, 0, 2, 0, 4, 0, 0, 2, 5, 0, 0, 0, 1});

	std::wstring expectedWideString = L"[2 - 0 - 0 - 3] - "
									  L"[0 - 2 - 0 - 4] - "
									  L"[0 - 0 - 2 - 5] - "
									  L"[0 - 0 - 0 - 1]";

	ASSERT_EQ(matrix.toWstring(), expectedWideString);

	std::string expectedString = "[2 - 0 - 0 - 3] - "
								 "[0 - 2 - 0 - 4] - "
								 "[0 - 0 - 2 - 5] - "
								 "[0 - 0 - 0 - 1]";

	ASSERT_EQ(matrix.toString(), expectedString);
}

TEST_F(MatrixTest, MatrixOutputStream_wostream)
{
	spk::Matrix4x4 matrix = spk::Matrix4x4({2, 0, 0, 3, 0, 2, 0, 4, 0, 0, 2, 5, 0, 0, 0, 1});

	std::wostringstream woss;
	woss << matrix;
	std::wstring expected = L"[2 - 0 - 0 - 3] - "
							L"[0 - 2 - 0 - 4] - "
							L"[0 - 0 - 2 - 5] - "
							L"[0 - 0 - 0 - 1]";

	ASSERT_EQ(woss.str(), expected);
}

TEST_F(MatrixTest, MatrixOutputStream_ostream)
{
	spk::Matrix4x4 matrix = spk::Matrix4x4({2, 0, 0, 3, 0, 2, 0, 4, 0, 0, 2, 5, 0, 0, 0, 1});

	std::ostringstream oss;
	oss << matrix;
	std::string expected = "[2 - 0 - 0 - 3] - "
						   "[0 - 2 - 0 - 4] - "
						   "[0 - 0 - 2 - 5] - "
						   "[0 - 0 - 0 - 1]";

	ASSERT_EQ(oss.str(), expected);
}

TEST_F(MatrixTest, RotationMatrix)
{
	spk::Vector3 position = spk::Vector3(1, 0, 0);

	ASSERT_EQ(spk::Matrix4x4::rotation(90, 0, 0) * position, spk::Vector3(1, 0, 0)) << "Wrong matrix multiplication";

	ASSERT_EQ(spk::Matrix4x4::rotation(0, 90, 0) * position, spk::Vector3(0, 0, -1)) << "Wrong matrix multiplication";

	ASSERT_EQ(spk::Matrix4x4::rotation(0, 0, 90) * position, spk::Vector3(0, 1, 0)) << "Wrong matrix multiplication";
}

TEST_F(MatrixTest, TranslationMatrix)
{
	spk::Vector3 position = spk::Vector3(1, 0, 0);

	spk::Vector3 translatedPosition = spk::Matrix4x4::translation(1, 2, 3) * position;

	ASSERT_EQ(translatedPosition, spk::Vector3(2, 2, 3)) << "Wrong matrix multiplication for translation";

	translatedPosition = spk::Matrix4x4::translation(0, 0, 0) * position;

	ASSERT_EQ(translatedPosition, position) << "Wrong matrix multiplication for identity translation";
}

TEST_F(MatrixTest, ScaleMatrix)
{
	spk::Vector3 position = spk::Vector3(1, 1, 1);

	spk::Vector3 scaledPosition = spk::Matrix4x4::scale(2, 3, 4) * position;

	ASSERT_EQ(scaledPosition, spk::Vector3(2, 3, 4)) << "Wrong matrix multiplication for scaling";

	scaledPosition = spk::Matrix4x4::scale(1, 1, 1) * position;

	ASSERT_EQ(scaledPosition, position) << "Wrong matrix multiplication for identity scaling";
}

TEST_F(MatrixTest, LookAtMatrix)
{
	spk::Vector3 from = spk::Vector3(2, 2, 2);
	spk::Vector3 to = spk::Vector3(2, 2, -1);
	spk::Vector3 up = spk::Vector3(0, 1, 0);

	spk::Matrix4x4 lookAtMatrix = spk::Matrix4x4::lookAt(from, to, up);

	spk::Vector3 result = lookAtMatrix * spk::Vector3(1, -1, 0);

	ASSERT_EQ(result, spk::Vector3(-1, -3, -2)) << "Wrong lookAt matrix transformation";
}

TEST_F(MatrixTest, RotateAroundAxisMatrix)
{
	spk::Vector3 axis = spk::Vector3(0, 1, 0);
	float angle = 90.0f;

	spk::Matrix4x4 rotateMatrix = spk::Matrix4x4::rotateAroundAxis(axis, angle);

	spk::Vector3 result = rotateMatrix * spk::Vector3(1, 0, 0);

	ASSERT_EQ(result, spk::Vector3(0, 0, 1)) << "Wrong rotateAroundAxis matrix transformation";
}

TEST_F(MatrixTest, PerspectiveMatrix)
{
	float fov = 90.0f;
	float aspectRatio = 1.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	spk::Matrix4x4 perspectiveMatrix = spk::Matrix4x4::perspective(fov, aspectRatio, nearPlane, farPlane);

	std::vector<std::pair<spk::Vector3, spk::Vector3>> values = {
		{spk::Vector3(0.0f, 0.0f, 0.11f), spk::Vector3(0.0f, 0.0f, 2.822f)},
		{spk::Vector3(0.0f, 0.0f, -1.0f), spk::Vector3(0.0f, 0.0f, 0.801802f)},
		{spk::Vector3(1.0f, 1.0f, -10.0f), spk::Vector3(0.1f, 0.1f, 0.981982f)},
		{spk::Vector3(-1.0f, -1.0f, -10.0f), spk::Vector3(-0.1f, -0.1f, 0.981982f)},
		{spk::Vector3(10.0f, 10.0f, -10.0f), spk::Vector3(1, 1, 0.981982)},
		{spk::Vector3(-10.0f, -10.0f, -10.0f), spk::Vector3(-1, -1, 0.981982)},
		{spk::Vector3(100.0f, 100.0f, -10.0f), spk::Vector3(10.0f, 10.0f, 0.981982f)},
		{spk::Vector3(-100.0f, -100.0f, -10.0f), spk::Vector3(-10.0f, -10.0f, 0.981982f)}};

	ASSERT_EQ(perspectiveMatrix * values[0].first, values[0].second) << "Error in computed perspective position [" << 0 << "]";
	ASSERT_EQ(perspectiveMatrix * values[1].first, values[1].second) << "Error in computed perspective position [" << 1 << "]";
	ASSERT_EQ(perspectiveMatrix * values[2].first, values[2].second) << "Error in computed perspective position [" << 2 << "]";
	ASSERT_EQ(perspectiveMatrix * values[3].first, values[3].second) << "Error in computed perspective position [" << 3 << "]";
	ASSERT_EQ(perspectiveMatrix * values[4].first, values[4].second) << "Error in computed perspective position [" << 4 << "]";
	ASSERT_EQ(perspectiveMatrix * values[5].first, values[5].second) << "Error in computed perspective position [" << 5 << "]";
	ASSERT_EQ(perspectiveMatrix * values[6].first, values[6].second) << "Error in computed perspective position [" << 6 << "]";
	ASSERT_EQ(perspectiveMatrix * values[7].first, values[7].second) << "Error in computed perspective position [" << 7 << "]";
}

TEST_F(MatrixTest, OrthoMatrix)
{
	float left = -10.0f;
	float right = 10.0f;
	float bottom = -10.0f;
	float top = 10.0f;
	float nearPlane = -1.0f;
	float farPlane = 1.0f;

	spk::Matrix4x4 orthoMatrix = spk::Matrix4x4::ortho(left, right, bottom, top, nearPlane, farPlane);

	std::vector<std::pair<spk::Vector3, spk::Vector3>> values = {
		{spk::Vector3(5, 5, 0), spk::Vector3(0.5f, 0.5f, 0)},
		{spk::Vector3(5, -5, 0.25), spk::Vector3(0.5f, -0.5f, -0.25)},
		{spk::Vector3(-5, 5, 0.5), spk::Vector3(-0.5f, 0.5f, -0.5)},
		{spk::Vector3(-5, -5, 0.75), spk::Vector3(-0.5f, -0.5f, -0.75)}};

	for (size_t i = 0; i < values.size(); i++)
	{
		ASSERT_EQ(orthoMatrix * values[i].first, values[i].second) << "Error in computed ortho position [" << i << "]";
	}
}

TEST_F(MatrixTest, DeterminantAndInverse)
{
	spk::Matrix2x2 matrix = spk::Matrix2x2({4.0f, 2.0f, 7.0f, 6.0f});

	EXPECT_NEAR(matrix.determinant(), 10.0f, 1e-6f) << "Determinant should match the analytical value";
	EXPECT_TRUE(matrix.isInvertible()) << "Matrix with non-zero determinant should be invertible";

	spk::Matrix2x2 inverse = matrix.inverse();
	spk::Matrix2x2 identity = matrix * inverse;

	for (size_t column = 0; column < 2; ++column)
	{
		for (size_t row = 0; row < 2; ++row)
		{
			const float expected = (column == row ? 1.0f : 0.0f);
			EXPECT_NEAR(identity[column][row], expected, 1e-5f) << "Product with inverse should yield the identity matrix";
		}
	}
}

TEST_F(MatrixTest, NonInvertibleMatrix)
{
	spk::Matrix3x3 matrix = spk::Matrix3x3({
		1.0f,
		0.0f,
		0.0f,
		2.0f,
		0.0f,
		0.0f,
		3.0f,
		0.0f,
		0.0f,
	});

	EXPECT_FLOAT_EQ(matrix.determinant(), 0.0f) << "Determinant should be zero for singular matrix";
	EXPECT_FALSE(matrix.isInvertible()) << "Matrix with zero determinant should not be invertible";
	EXPECT_THROW(matrix.inverse(), std::runtime_error) << "Inversion should throw for singular matrix";
}