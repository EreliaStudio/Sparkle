#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "math/perlin/spk_perlin_2D.hpp"

using namespace spk;

class Perlin2DTest : public ::testing::Test {
protected:
    Perlin2D perlinNoise;

	Perlin2DTest() :
		perlinNoise(12345)
	{

	}

    virtual void SetUp() override {
        perlinNoise.configureFrequency(10.0f);
        perlinNoise.configurePersistance(0.5f);
        perlinNoise.configureLacunarity(2.0f);
        perlinNoise.configureOctave(4);
        perlinNoise.configureRange(-100.0f, 100.0f);
        perlinNoise.configureInterpolation(IPerlin::Interpolation::Linear);
    }
};

TEST_F(Perlin2DTest, SampleConsistency) {
    float sample1 = perlinNoise.sample(0.5f, 0.5f);
    float sample2 = perlinNoise.sample(0.5f, 0.5f);

    ASSERT_NEAR(sample1, sample2, 1e-5);
}

TEST_F(Perlin2DTest, SampleVariation) {
    float sample1 = perlinNoise.sample(0.5f, 0.5f);
    float sample2 = perlinNoise.sample(0.6f, 0.5f);
    float sample3 = perlinNoise.sample(0.5f, 0.6f);

    ASSERT_NE(sample1, sample2);
    ASSERT_NE(sample1, sample3);
}

TEST_F(Perlin2DTest, SampleRange) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            float sample = perlinNoise.sample(static_cast<float>(i), static_cast<float>(j));
            ASSERT_GE(sample, -100.0f);
            ASSERT_LE(sample, 100.0f);
        }
    }
}

TEST_F(Perlin2DTest, VectorSample) {
    Vector2 position(0.5f, 0.5f);
    float sample1 = perlinNoise.sample(position);
    float sample2 = perlinNoise.sample(0.5f, 0.5f);

    ASSERT_NEAR(sample1, sample2, 1e-5);
}