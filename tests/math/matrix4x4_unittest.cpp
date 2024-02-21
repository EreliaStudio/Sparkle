
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

RC_GTEST_PROP(Matrix4x4Test, Addition, (const Matrix4x4& a, const Matrix4x4& b, const Matrix4x4& c))
{
    RC_ASSERT((a + b) == (b + a));
    RC_ASSERT(((a + b) + c) == (a + (b + c)));
}

RC_GTEST_PROP(Matrix4x4Test, Transpose, (const Matrix4x4& a))
{
    RC_ASSERT(a.transpose().transpose() == a);
}

RC_GTEST_PROP(Matrix4x4Test, Mult, (const Matrix4x4& a))
{
    RC_ASSERT((a * Matrix4x4()) == a);
    RC_ASSERT((a * Matrix4x4::zero()) == Matrix4x4::zero);
}

RC_GTEST_PROP(Matrix4x4Test, Mult3, (const Matrix4x4& a, const Matrix4x4& b, const Matrix4x4& c))
{
    RC_ASSERT((a * (b * c)) == ((a * b) * c));
    RC_ASSERT((a * (b + c)) == ((a * b) + (a * c)));
    RC_ASSERT(((a + b) * c) == ((a * c) + (b * c)));
}

RC_GTEST_PROP(Matrix4x4Test, InverseMult, (const Matrix4x4& a))
{
    // RC_ASSERT(((a * b) * (b.inverse())) == a);
    // RC_ASSERT((a * a.inverse()) == (a.inverse() * a));
    RC_ASSERT((a * a.inverse()) == Matrix4x4());
}