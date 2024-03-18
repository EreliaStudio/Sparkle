#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "math/perlin/spk_perlin_3D.hpp"

using namespace spk;

class Perlin3DTest : public ::testing::Test {
protected:
    Perlin3D perlinNoise;

	Perlin3DTest() :
		perlinNoise(12345)
	{

	}

    virtual void SetUp() override {
        perlinNoise.configureFrequency(10.0f);
        perlinNoise.configurePersistance(0.5f);
        perlinNoise.configureLacunarity(2.0f);
        perlinNoise.configureOctave(4);
        perlinNoise.configureRange(-100.0f, 100.0f);
        perlinNoise.configureInterpolation(IPerlin::Interpolation::SmoothStep);
    }
};

TEST_F(Perlin3DTest, SampleNoiseValue) {
    // Sample at a specific point
    float noiseValue = perlinNoise.sample(1.0f, 1.0f, 1.0f);
    // Basic check to ensure the noise value is within the expected range
    ASSERT_GE(noiseValue, -100.0f);
    ASSERT_LE(noiseValue, 100.0f);
}

TEST_F(Perlin3DTest, Consistency) {
    // Sample at two different points
    float noiseValue1 = perlinNoise.sample(1.0f, 1.0f, 1.0f);
    float noiseValue2 = perlinNoise.sample(1.0f, 1.0f, 1.0f);

    // The noise values should be consistent when sampled at the same point
    ASSERT_FLOAT_EQ(noiseValue1, noiseValue2);
}

TEST_F(Perlin3DTest, DifferentValues) {
    // Sample at two different points
    float noiseValue1 = perlinNoise.sample(1.0f, 1.0f, 1.0f);
    float noiseValue2 = perlinNoise.sample(2.0f, 2.0f, 2.0f);

    // It's highly unlikely that two different points produce the exact same noise value
    ASSERT_NE(noiseValue1, noiseValue2);
}

TEST_F(Perlin3DTest, TemplateSampleMethod) {
    // Use the templated sample method with a Vector3
    Vector3 position(1.0f, 2.0f, 3.0f);
    float noiseValue = perlinNoise.sample(position);

    // Perform a basic check to ensure the noise value is within the expected range
    ASSERT_GE(noiseValue, -100.0f);
    ASSERT_LE(noiseValue, 100.0f);
}
