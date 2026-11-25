#include <gtest/gtest.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "math/domain_warp.hpp"
#include "math/perlin.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/paths.hpp"

namespace
{
	enum class Field
	{
		Raw,
		Threshold,
		ThreeDimensional,
		Warped
	};

	void captureNoise(const std::string &name, Field field, int octaves, spk::Perlin::Interpolation interpolation = spk::Perlin::Interpolation::Quintic)
	{
		constexpr int Width = 640;
		constexpr int Height = 480;
		spk::Perlin noise(118, interpolation);
		noise.setFrequency(0.055f);
		noise.setOctaves(octaves);
		spk::DomainWarp warp(931);
		warp.setAmplitude(12);
		warp.xNoise().setFrequency(0.039f);
		warp.yNoise().setFrequency(0.047f);
		std::vector<std::uint8_t> pixels(Width * Height * 4);
		for (int z = 0; z < Height; ++z)
		{
			for (int x = 0; x < Width; ++x)
			{
				spk::Vector2 position{static_cast<float>(x) / 8.0f - 32.0f, static_cast<float>(z) / 6.0f - 32.0f};
				if (field == Field::Warped)
				{
					position = warp.warp2D(position);
				}
				const float value = field == Field::ThreeDimensional ? noise.sample3D(position.x, 13.375f, position.y) : noise.sample2D(position.x, position.y);
				const bool threshold = field == Field::Threshold || field == Field::Warped;
				const auto shade = static_cast<std::uint8_t>(std::round(std::clamp(value, 0.0f, 1.0f) * 255));
				const auto index = static_cast<std::size_t>((z * Width + x) * 4);
				pixels[index] = threshold ? (value < 0.5f ? 142 : 151) : shade;
				pixels[index + 1] = threshold ? (value < 0.5f ? 103 : 162) : shade;
				pixels[index + 2] = threshold ? (value < 0.5f ? 70 : 175) : shade;
				pixels[index + 3] = 255;
			}
		}
		const auto actual = sparkle_test::resultImagePath("rendering/perlin-noise", name);
		std::filesystem::create_directories(actual.parent_path());
		ASSERT_NE(stbi_write_png(actual.string().c_str(), Width, Height, 4, pixels.data(), Width * 4), 0);
		const auto expected = sparkle_test::expectedImagePath("rendering/perlin-noise", name);
		ASSERT_TRUE(std::filesystem::exists(expected)) << "Missing approved noise golden: " << actual;
		sparkle_test::ImageComparisonOptions tolerance;
		tolerance.rgbTolerance = 1;
		tolerance.alphaTolerance = 0;
		const auto comparison = sparkle_test::compareImages(actual, expected, sparkle_test::resultImagePath("rendering/perlin-noise", name + "_difference"), tolerance);
		EXPECT_TRUE(comparison.matches) << name << ": " << comparison.differentPixelCount << " different pixels";
	}
}

TEST(PerlinNoiseGolden, RawSingleOctave)
{
	captureNoise("raw_single_octave", Field::Raw, 1);
}

TEST(PerlinNoiseGolden, ThresholdSingleOctave)
{
	captureNoise("threshold_single_octave", Field::Threshold, 1);
}

TEST(PerlinNoiseGolden, RawThreeOctaves)
{
	captureNoise("raw_three_octaves", Field::Raw, 3);
}

TEST(PerlinNoiseGolden, ThresholdThreeOctaves)
{
	captureNoise("threshold_three_octaves", Field::Threshold, 3);
}

TEST(PerlinNoiseGolden, CubicThreshold)
{
	captureNoise("threshold_cubic", Field::Threshold, 1, spk::Perlin::Interpolation::SmoothStep);
}

TEST(PerlinNoiseGolden, ThreeDimensionalSlice)
{
	captureNoise("three_dimensional_slice", Field::ThreeDimensional, 3);
}

TEST(PerlinNoiseGolden, WarpedThreshold)
{
	captureNoise("threshold_domain_warp", Field::Warped, 3);
}
