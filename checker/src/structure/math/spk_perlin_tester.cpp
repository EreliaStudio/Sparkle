#include "structure/math/spk_perlin_tester.hpp"

#include <cmath>

TEST_F(PerlinTest, DeterministicForSeed)
{
	spk::Perlin noise(1234u, spk::Perlin::Interpolation::SmoothStep);
	float a = noise.sample1D(0.5f, -1.0f, 1.0f);
	float b = noise.sample1D(0.5f, -1.0f, 1.0f);

	EXPECT_FLOAT_EQ(a, b);
	EXPECT_GE(a, -1.0f);
	EXPECT_LE(a, 1.0f);
}

TEST_F(PerlinTest, DifferentSeedsProduceDifferentValues)
{
	spk::Perlin smooth(1337u, spk::Perlin::Interpolation::SmoothStep);
	spk::Perlin linear(4242u, spk::Perlin::Interpolation::Linear);

	float v1 = smooth.sample2D(0.25f, 0.75f, 0.0f, 1.0f);
	float v2 = linear.sample2D(0.25f, 0.75f, 0.0f, 1.0f);

	EXPECT_GE(v1, 0.0f);
	EXPECT_LE(v1, 1.0f);
	EXPECT_GE(v2, 0.0f);
	EXPECT_LE(v2, 1.0f);
	EXPECT_GT(std::fabs(v1 - v2), 1e-6f);
}

TEST_F(PerlinTest, Sample3DIsInRange)
{
	spk::Perlin noise(2024u, spk::Perlin::Interpolation::SmoothStep);

	float min = -5.0f;
	float max = 3.0f;
	float value = noise.sample3D(0.1f, 0.2f, 0.3f, min, max);

	EXPECT_GE(value, min);
	EXPECT_LE(value, max);
}
