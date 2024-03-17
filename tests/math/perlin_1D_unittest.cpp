#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "math/perlin/spk_perlin_1D.hpp"

using namespace spk;

class Perlin1DTest : public ::testing::Test {
protected:
    Perlin1D perlinDefault;
    Perlin1D perlinWithSeed;

	Perlin1DTest() :
		perlinWithSeed(Perlin1D(12345))
	{

	}

    virtual void SetUp() override
	{
		perlinDefault.configureFrequency(10.0f);
		perlinDefault.configurePersistance(0.5f);
		perlinDefault.configureLacunarity(2.0f);
		perlinDefault.configureOctave(4);
		perlinDefault.configureRange(0.0f, 100.0f);
		perlinDefault.configureInterpolation(IPerlin::Interpolation::Linear);

		perlinWithSeed.configureFrequency(10.0f);
		perlinWithSeed.configurePersistance(0.5f);
		perlinWithSeed.configureLacunarity(2.0f);
		perlinWithSeed.configureOctave(4);
		perlinWithSeed.configureRange(0.0f, 100.0f);
		perlinWithSeed.configureInterpolation(IPerlin::Interpolation::Linear);
    }
};

TEST_F(Perlin1DTest, Initialization) {
    // Test if the object initialized correctly
    ASSERT_NO_THROW(Perlin1D());
    ASSERT_NO_THROW(Perlin1D(12345));
}

TEST_F(Perlin1DTest, SampleRange) {
    perlinWithSeed.configureRange(-1.0f, 1.0f);
    for (float x = 0.0f; x <= 10.0f; x += 1.0f) {
        float value = perlinWithSeed.sample(x);
        ASSERT_GE(value, -1.0f);
        ASSERT_LE(value, 1.0f);
    }
}

TEST_F(Perlin1DTest, ConsistencyWithSeed) {
    // Check if the same seed produces the same results
    Perlin1D anotherPerlinWithSeed(12345);

	anotherPerlinWithSeed.configureFrequency(10.0f);
	anotherPerlinWithSeed.configurePersistance(0.5f);
	anotherPerlinWithSeed.configureLacunarity(2.0f);
	anotherPerlinWithSeed.configureOctave(4);
	anotherPerlinWithSeed.configureRange(0.0f, 100.0f);
	anotherPerlinWithSeed.configureInterpolation(IPerlin::Interpolation::Linear);

    for (float x = 0.0f; x <= 10.0f; x += 1.0f) {
        ASSERT_EQ(perlinWithSeed.sample(x), anotherPerlinWithSeed.sample(x));
    }
}

TEST_F(Perlin1DTest, ConfigurationEffect) {
    // Modify the configuration and check if it affects the output
    perlinWithSeed.configureFrequency(10.0f);
    perlinWithSeed.configurePersistance(0.5f);
    perlinWithSeed.configureLacunarity(2.0f);
    perlinWithSeed.configureOctave(4);
    perlinWithSeed.configureRange(-1.0f, 1.0f);
    perlinWithSeed.configureInterpolation(IPerlin::Interpolation::Linear);

    float originalSample = perlinWithSeed.sample(5.0f);
    
    // Change a configuration and check if the output changes
    perlinWithSeed.configureFrequency(100.0f);
    ASSERT_NE(perlinWithSeed.sample(5.0f), originalSample);
}
