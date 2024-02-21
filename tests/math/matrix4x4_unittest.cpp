
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

RC_GTEST_PROP(Matrix4x4Test, Equality, ())
{
    Matrix4x4 first = spk::Matrix4x4::scaleMatrix(spk::Vector3(2, 2, 2));
    Matrix4x4 second = spk::Matrix4x4(
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 2, 0, 
        0, 0, 0, 1
    );
    Matrix4x4 third = spk::Matrix4x4::scaleMatrix(spk::Vector3(3, 3, 3));
    
    RC_ASSERT((first) == (second));
    RC_ASSERT((second) == (first));
    RC_ASSERT(!((first) == (third)));
    RC_ASSERT(!((second) == (third)));
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
    RC_ASSERT((a * Matrix4x4::zero()) == Matrix4x4::zero());
}

RC_GTEST_PROP(Matrix4x4Test, Mult3, (const Matrix4x4& a, const Matrix4x4& b, const Matrix4x4& c))
{
    RC_ASSERT((a * spk::Matrix4x4()) == a);

    Matrix4x4 first = spk::Matrix4x4(
        25, 0, 0, 0,
        0, -26, 0, 0,
        0, 0, 2, 0,
        0, 0, 0, 1
    );

    Matrix4x4 second = spk::Matrix4x4(
        1, 0, 0, 0,
        0, 1, 0, 0, 
        0, 0, 1, 0,
        -1, 1, 0, 1
    );

    spk::Matrix4x4 expected = spk::Matrix4x4(
        25,   0,  0,  0, 
        0, -26,  0,  0,
        0,   0,  2,  0, 
        -1, 1, 0, 1
    );

    RC_ASSERT((first * second) == expected);


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