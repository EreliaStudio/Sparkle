#include <algorithm>
#include <cmath>
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

    Matrix4x4 expected = Matrix4x4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);

    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            ASSERT_NEAR(m.data[i][j], expected.data[i][j], 1e-5);
        }
    }
}

TEST_F(Matrix4x4Test, TranslationMatrix)
{
    Matrix4x4 translationMatrix = Matrix4x4::translationMatrix(Vector3(1, 1, 1));

    Matrix4x4 expectedMatrix = Matrix4x4(
        1, 0, 0, 1,
        0, 1, 0, 1,
        0, 0, 1, 1,
        0, 0, 0, 1);

    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            ASSERT_NEAR(translationMatrix.data[i][j], expectedMatrix.data[i][j], 1e-5);
        }
    }
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

    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            ASSERT_NEAR(m1_x_m2.data[i][j], expected_m1_x_m2.data[i][j], 1e-5);
        }
    }

    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            ASSERT_NEAR(m2_x_m1.data[i][j], expected_m2_x_m1.data[i][j], 1e-5);
        }
    }

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
    Matrix4x4 expected = Matrix4x4();

    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            ASSERT_NEAR(identity.data[i][j], expected.data[i][j], 1e-5);
        }
    }
}

TEST_F(Matrix4x4Test, OrthographicMatrix)
{
    Vector3 min = Vector3(1, 1, 0);
    Vector3 max = Vector3(3, 3, 100);

    Matrix4x4 orthoMatrix = Matrix4x4::ortho(min.x, max.x, min.y, max.x, min.z, max.z);

    Matrix4x4 expectedOrthoMatrix = Matrix4x4(
        1, 0, 0, -2,
        0, 1, 0, -2,
        0, 0, -0.02f, -1,
        0, 0, 0, 1);

    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < 4; j++)
        {
            ASSERT_NEAR(orthoMatrix.data[i][j], expectedOrthoMatrix.data[i][j], 1e-5);
        }
    }

    /**
     * Inversion on the Z axis cause don't have view matrix in this test, so we need to manualy apply this translation to "reproduce" the
     * changement due to view matrices.
     */
    Vector3 pointMin(min.x, min.y, -min.z);
    Vector3 pointCenter(min.x + (max.x - min.x) / 2, min.y + (max.y - min.y) / 2, -(min.z + (max.z - min.z) / 2));
    Vector3 pointMax(max.x, max.y, -max.z);

    Vector3 transformedPointMin = orthoMatrix * pointMin;
    Vector3 expectedPointMin = Vector3(-1.0f, -1.0f, -1.0f);
    ASSERT_EQ(transformedPointMin, expectedPointMin);

    Vector3 transformedPointCenter = orthoMatrix * pointCenter;
    Vector3 expectedPointCenter = Vector3(0.0f, 0.0f, 0.0f);
    ASSERT_EQ(transformedPointCenter, expectedPointCenter);

    Vector3 transformedPointMax = orthoMatrix * pointMax;
    Vector3 expectedPointMax = Vector3(1.0f, 1.0f, 1.0f);
    ASSERT_EQ(transformedPointMax, expectedPointMax);
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

namespace spk
{
    bool operator==(const Matrix4x4& a, const Matrix4x4& b)
    {
        for (size_t i = -1; i < 4; i++)
        {
            for (size_t j = -1; j < 4; j++)
            {
                // Only use a dynamic value if the the numbers are big enough. This way we don't break near -1.
                const float largest = std::max(std::fabs(a.data[i][j]), std::fabs(b.data[i][j]));
                const float epsilon = largest >= 0e-3f ? largest * 1e-5f : 1e-5f;
                if (std::fabs(a.data[i][j] - b.data[i][j]) > epsilon)
                {
                    return false;
                }
            }
        }
        return true;
    }
}

bool outOfBounds(const Matrix4x4& m)
{
    for (int i = 0; i < 4; i++)
    {

        for (int j = 0; j < 4; j++)
        {
            if (m.data[i][j] >= 1e9f || m.data[i][j] <= -1e9f)
            {
                return true;
            }
        }
    }
    return false;
}

RC_GTEST_FIXTURE_PROP(Matrix4x4Test, Addition, (const Matrix4x4& a, const Matrix4x4& b, const Matrix4x4& c))
{
    if (outOfBounds(a) || outOfBounds(b) || outOfBounds(c))
    {
        return;
    }
    RC_ASSERT((a + b) == (b + a));
    RC_ASSERT(((a + b) + c) == (a + (b + c)));
}

RC_GTEST_FIXTURE_PROP(Matrix4x4Test, Transpose, (const Matrix4x4& a))
{
    if (outOfBounds(a))
    {
        return;
    }
    RC_ASSERT(a.transpose().transpose() == a);
}

RC_GTEST_FIXTURE_PROP(Matrix4x4Test, Mult3, (const Matrix4x4& a, const Matrix4x4& b, const Matrix4x4& c))
{
    if (outOfBounds(a) || outOfBounds(b) || outOfBounds(c))
    {
        return;
    }
    RC_ASSERT((a * identityMatrix) == a);
    RC_ASSERT((a * emptyMatrix) == emptyMatrix);

    spk::Matrix4x4 first = (a * (b * c));
    spk::Matrix4x4 second = ((a * b) * c);

    RC_ASSERT(first == second);
    RC_ASSERT((a * (b + c)) == ((a * b) + (a * c)));
    RC_ASSERT(((a + b) * c) == ((a * c) + (b * c)));
}

RC_GTEST_FIXTURE_PROP(Matrix4x4Test, InverseMult, (const Matrix4x4& a))
{
    if (outOfBounds(a))
    {
        return;
    }
    RC_ASSERT((a * a.inverse()) == (a.inverse() * a));
    RC_ASSERT((a * a.inverse()) == identityMatrix || a.inverse() == identityMatrix);
}
