
#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "math/spk_matrix4x4.hpp"

using spk::Matrix4x4;

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

RC_GTEST_PROP(Matrix4x4Test, Transpose, (const Matrix4x4& a))
{
    RC_ASSERT(a.transpose().transpose() == a);
}

RC_GTEST_PROP(Matrix4x4Test, Mult, (const Matrix4x4& a))
{
    RC_ASSERT((a * Matrix4x4()) == a);
    auto res = a * Matrix4x4::scaleMatrix(2);
    RC_ASSERT(res == Matrix4x4(
                         2 * a.data[0][0],
                         2 * a.data[0][1],
                         2 * a.data[0][2],
                         2 * a.data[0][3],
                         2 * a.data[1][0],
                         2 * a.data[1][1],
                         2 * a.data[1][2],
                         2 * a.data[1][3],
                         2 * a.data[2][0],
                         2 * a.data[2][1],
                         2 * a.data[2][2],
                         2 * a.data[2][3],
                         2 * a.data[3][0],
                         2 * a.data[3][1],
                         2 * a.data[3][2],
                         a.data[3][3]));
}

RC_GTEST_PROP(Matrix4x4Test, InverseMult, (const Matrix4x4& a, const Matrix4x4& b))
{
    RC_ASSERT(((a * b) * (b.inverse())) == a);
    RC_ASSERT((a * a.inverse()) == Matrix4x4());
}