
#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "math/spk_matrix4x4.hpp"
#include "spk_basic_functions.hpp"

using namespace spk;

class Matrix4x4Test : public ::testing::Test
{
protected:
    Matrix4x4 identityMatrix;
    Matrix4x4 emptyMatrix;

    virtual void SetUp() override
    {
        identityMatrix = Matrix4x4();
        emptyMatrix = Matrix4x4(
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0);
    }
};

TEST_F(Matrix4x4Test, Constructor_Default)
{
    Matrix4x4 m;
    ASSERT_EQ(m, identityMatrix);
}

TEST_F(Matrix4x4Test, TranslationMatrix)
{
    Matrix4x4 translationMatrix = Matrix4x4::translationMatrix(Vector3(1, 1, 1));

    Matrix4x4 expectedMatrix = Matrix4x4(
        1, 0, 0, 1,
        0, 1, 0, 1,
        0, 0, 1, 1,
        0, 0, 0, 1);

    ASSERT_EQ(translationMatrix, expectedMatrix);
}

TEST_F(Matrix4x4Test, Multiplication_Vector3)
{
    Vector3 v(1, 2, 3);
    Matrix4x4 m = Matrix4x4::translationMatrix(Vector3(1, 1, 1));

    Vector3 result = m * v;
    Vector3 expectedPoint(2, 3, 4);

    ASSERT_EQ(result, expectedPoint);
}

TEST_F(Matrix4x4Test, Multiplication_Matrix)
{
    Matrix4x4 m1 = Matrix4x4::translationMatrix(Vector3(1, 2, 3));
    Matrix4x4 m2 = Matrix4x4::scaleMatrix(Vector3(2, 2, 2));

    Matrix4x4 m1_x_m2 = m1 * m2;
    Matrix4x4 m2_x_m1 = m2 * m1;

    Matrix4x4 expected_m1_x_m2 = Matrix4x4(
        2, 0, 0, 2,
        0, 2, 0, 4,
        0, 0, 2, 6,
        0, 0, 0, 1);

    Matrix4x4 expected_m2_x_m1 = Matrix4x4(
        2, 0, 0, 1,
        0, 2, 0, 2,
        0, 0, 2, 3,
        0, 0, 0, 1);

    ASSERT_EQ(m1_x_m2, expected_m1_x_m2);
    ASSERT_EQ(m2_x_m1, expected_m2_x_m1);

    Vector3 point(2, 2, 2);

    Vector3 result = expected_m1_x_m2 * point;
    Vector3 expectedPoint(6, 8, 10);

    Vector3 result2 = expected_m2_x_m1 * point;
    Vector3 expectedPoint2(5, 6, 7);

    ASSERT_EQ(result, expectedPoint);
    ASSERT_EQ(result2, expectedPoint2);
}

TEST_F(Matrix4x4Test, ScaleMatrix)
{
    Matrix4x4 scaleMatrix = Matrix4x4::scaleMatrix(Vector3(2, 3, 4));

    Vector3 v(2, 2, 2);

    Vector3 result = scaleMatrix * v;
    Vector3 expected = Vector3(4, 6, 8);

    ASSERT_EQ(result, expected);
}

TEST_F(Matrix4x4Test, RotationMatrix_XAxis)
{
    Matrix4x4 rotationMatrix = Matrix4x4::rotationMatrix(Vector3(90, 0, 0));
    Vector3 point(0, 1, 0);

    Vector3 result = rotationMatrix * point;
    Vector3 expected = Vector3(0, 0, -1);
    ASSERT_EQ(result, expected);
}

TEST_F(Matrix4x4Test, RotationMatrix_YAxis)
{
    Vector3 point(1, 0, 0);
    Matrix4x4 rotationMatrix = Matrix4x4::rotationMatrix(Vector3(0, 90, 0));

    Vector3 result = rotationMatrix * point;
    Vector3 expected = Vector3(0, 0, 1);

    ASSERT_EQ(result, expected);
}

TEST_F(Matrix4x4Test, RotationMatrix_ZAxis)
{
    Vector3 point(0, 1, 0);
    Matrix4x4 rotationMatrix = Matrix4x4::rotationMatrix(Vector3(0, 0, 90));

    Vector3 result = rotationMatrix * point;
    Vector3 expected = Vector3(1, 0, 0);

    ASSERT_EQ(result, expected);
}

TEST_F(Matrix4x4Test, InverseMatrix)
{
    Matrix4x4 m = Matrix4x4::translationMatrix(Vector3(5, 6, 7)) * Matrix4x4::rotationMatrix(Vector3(M_PI * 2, M_PI, M_PI * 4)) * Matrix4x4::scaleMatrix(Vector3(2, 2, 2));
    Matrix4x4 inverseM = m.inverse();
    Matrix4x4 identity = m * inverseM;

    ASSERT_EQ(Matrix4x4(), identity);
}

TEST_F(Matrix4x4Test, OrthographicMatrix)
{
    Matrix4x4 orthoMatrix = Matrix4x4::ortho(1, 3, 1, 3, 50, 150);

    Matrix4x4 expectedOrthoMatrix = Matrix4x4(
        1, 0, 0, -2,
        0, 1, 0, -2,
        0, 0, -0.02f, -2,
        0, 0, 0, 1);

    ASSERT_EQ(orthoMatrix, expectedOrthoMatrix);

    Vector3 point(2.5f, 2.5f, 100.0f);

    Vector3 transformedPoint = orthoMatrix * point;
    Vector3 expectedPoint = Vector3(0.5f, 0.5f, 0.0f); // On both axis, we work from -1 to 1

    ASSERT_NEAR(transformedPoint.x, expectedPoint.x, 1e-5);
    ASSERT_NEAR(transformedPoint.y, expectedPoint.y, 1e-5);
    ASSERT_NEAR(transformedPoint.z, expectedPoint.z, 1e-5);
}

/**
 * Property-based tests
 */

namespace rc
{

    template <>
    struct Arbitrary<Matrix4x4>
    {
        static Gen<Matrix4x4> arbitrary()
        {
            return gen::construct<Matrix4x4>(
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>(),
                gen::arbitrary<float>());
        }
    };
}

RC_GTEST_FIXTURE_PROP(Matrix4x4Test, Addition, (const Matrix4x4& a, const Matrix4x4& b, const Matrix4x4& c))
{
    RC_ASSERT((a + b) == (b + a));
    RC_ASSERT(((a + b) + c) == (a + (b + c)));
}

RC_GTEST_FIXTURE_PROP(Matrix4x4Test, Transpose, (const Matrix4x4& a))
{
    RC_ASSERT(a.transpose().transpose() == a);
}

RC_GTEST_FIXTURE_PROP(Matrix4x4Test, Mult3, (const Matrix4x4& a, const Matrix4x4& b, const Matrix4x4& c))
{
    RC_ASSERT((a * identityMatrix) == a);
    RC_ASSERT((a * emptyMatrix) == emptyMatrix);

    RC_ASSERT((a * (b * c)) == ((a * b) * c));
    RC_ASSERT((a * (b + c)) == ((a * b) + (a * c)));
    RC_ASSERT(((a + b) * c) == ((a * c) + (b * c)));
}

RC_GTEST_FIXTURE_PROP(Matrix4x4Test, InverseMult, (const Matrix4x4& a))
{
    RC_ASSERT((a * a.inverse()) == (a.inverse() * a));
    RC_ASSERT((a * a.inverse()) == identityMatrix);
}