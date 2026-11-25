#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

#include "math/domain_warp.hpp"
#include "math/perlin.hpp"

namespace
{
	constexpr float Tolerance = 1.0e-5f;

	template <typename TOperation>
	void expectInvalid(TOperation &&operation)
	{
		EXPECT_THROW(operation(), std::invalid_argument);
	}
}

TEST(PerlinTest, QuinticDefaultsAndSeedZeroIsReproducible)
{
	const spk::Perlin original;
	const spk::Perlin again(0u);
	EXPECT_EQ(original.seed(), 0u);
	EXPECT_EQ(original.interpolation(), spk::Perlin::Interpolation::Quintic);
	EXPECT_EQ(original.octaves(), 3);
	EXPECT_FLOAT_EQ(original.persistence(), 0.5f);
	EXPECT_FLOAT_EQ(original.lacunarity(), 2.0f);
	EXPECT_FLOAT_EQ(original.frequency(), 1.0f);

	for (float x : {-19.5f, -0.75f, 0.125f, 3.875f, 54.25f})
	{
		EXPECT_FLOAT_EQ(original.sample1D(x), again.sample1D(x));
		EXPECT_FLOAT_EQ(original.sample2D(x, 12.375f), again.sample2D(x, 12.375f));
		EXPECT_FLOAT_EQ(original.sample3D(x, -7.125f, 3.25f), again.sample3D(x, -7.125f, 3.25f));
	}
}

TEST(PerlinTest, DistinctSeedsAndReseedingChangeTheFieldPredictably)
{
	spk::Perlin first(1337u);
	const spk::Perlin copy(1337u);
	const spk::Perlin distinct(7331u);
	const auto sample = [](const spk::Perlin &noise) {
		return std::array<float, 5>{
			noise.sample1D(0.317f),
			noise.sample2D(-2.125f, 7.375f),
			noise.sample3D(1.8125f, -3.875f, 4.625f),
			noise.sample2D(27.3f, -19.1f),
			noise.sample3D(-13.7f, -2.4f, 5.9f)};
	};

	EXPECT_EQ(sample(first), sample(copy));
	EXPECT_NE(sample(first), sample(distinct));
	first.setSeed(7331u);
	EXPECT_EQ(first.seed(), 7331u);
	EXPECT_EQ(sample(first), sample(distinct));
	first.setSeed(1337u);
	EXPECT_EQ(sample(first), sample(copy));
}

TEST(PerlinTest, IntegerLatticePointsAlwaysMapToTheRangeMidpoint)
{
	spk::Perlin noise(42u);
	noise.setOctaves(1);
	for (float coordinate : {-65536.0f, -257.0f, -16.0f, -1.0f, 0.0f, 1.0f, 16.0f, 255.0f, 65536.0f})
	{
		EXPECT_FLOAT_EQ(noise.sample1D(coordinate), 0.5f);
		EXPECT_FLOAT_EQ(noise.sample2D(coordinate, -3.0f, -8.0f, 8.0f), 0.0f);
		EXPECT_FLOAT_EQ(noise.sample3D(coordinate, 2.0f, -7.0f, 4.0f, 14.0f), 9.0f);
	}
}

TEST(PerlinTest, NegativeCoordinatesAndChunkBoundariesAreContinuous)
{
	const spk::Perlin noise(291u);
	for (float boundary : {-48.0f, -32.0f, -16.0f, -1.0f, 0.0f, 1.0f, 16.0f, 32.0f, 48.0f})
	{
		SCOPED_TRACE(::testing::Message() << "boundary=" << boundary);
		EXPECT_NEAR(noise.sample1D(boundary - 0.0001f), noise.sample1D(boundary + 0.0001f), 0.002f);
		EXPECT_NEAR(
			noise.sample2D(boundary - 0.0001f, -4.375f),
			noise.sample2D(boundary + 0.0001f, -4.375f),
			0.002f);
		EXPECT_NEAR(
			noise.sample2D(-3.625f, boundary - 0.0001f),
			noise.sample2D(-3.625f, boundary + 0.0001f),
			0.002f);
		EXPECT_NEAR(
			noise.sample3D(3.375f, boundary - 0.0001f, -8.625f),
			noise.sample3D(3.375f, boundary + 0.0001f, -8.625f),
			0.002f);
	}
}

TEST(PerlinTest, LatticeIsPeriodicWithPeriod256ForOneOctave)
{
	spk::Perlin noise(1234u);
	noise.setOctaves(1);
	for (float x : {-21.25f, -0.625f, 0.375f, 11.875f})
	{
		EXPECT_NEAR(noise.sample1D(x), noise.sample1D(x + 256.0f), Tolerance);
		EXPECT_NEAR(noise.sample2D(x, -4.75f), noise.sample2D(x + 256.0f, -4.75f), Tolerance);
		EXPECT_NEAR(noise.sample3D(x, -4.75f, 9.625f), noise.sample3D(x, -4.75f, 9.625f + 256.0f), Tolerance);
	}
}

TEST(PerlinTest, AllDimensionsRespectRequestedOutputRanges)
{
	const spk::Perlin noise(54321u);
	for (int ix = -24; ix <= 24; ++ix)
	{
		const float x = static_cast<float>(ix) * 0.37f;
		const float first = noise.sample1D(x, -20.0f, 7.0f);
		EXPECT_TRUE(std::isfinite(first));
		EXPECT_GE(first, -20.0f);
		EXPECT_LE(first, 7.0f);

		for (int iy = -8; iy <= 8; ++iy)
		{
			const float y = static_cast<float>(iy) * 0.43f;
			const float second = noise.sample2D(x, y, -12.0f, 3.0f);
			const float third = noise.sample3D(x, y, 2.375f, -5.0f, 5.0f);
			EXPECT_TRUE(std::isfinite(second));
			EXPECT_TRUE(std::isfinite(third));
			EXPECT_GE(second, -12.0f);
			EXPECT_LE(second, 3.0f);
			EXPECT_GE(third, -5.0f);
			EXPECT_LE(third, 5.0f);
		}
	}
	EXPECT_FLOAT_EQ(noise.sample1D(0.9375f, 7.0f, 7.0f), 7.0f);
	EXPECT_FLOAT_EQ(noise.sample2D(0.9375f, -3.125f, -2.0f, -2.0f), -2.0f);
	EXPECT_FLOAT_EQ(noise.sample3D(0.9375f, -3.125f, 4.75f, 11.0f, 11.0f), 11.0f);
}

TEST(PerlinTest, InterpolationAndOctaveParametersActuallyChangeSamples)
{
	spk::Perlin noise(123u);
	noise.setOctaves(1);
	noise.setInterpolation(spk::Perlin::Interpolation::SmoothStep);
	const float smooth = noise.sample1D(0.213f);
	noise.setInterpolation(spk::Perlin::Interpolation::Linear);
	EXPECT_EQ(noise.interpolation(), spk::Perlin::Interpolation::Linear);
	EXPECT_NE(noise.sample1D(0.213f), smooth);
	noise.setInterpolation(spk::Perlin::Interpolation::SmoothStep);
	EXPECT_FLOAT_EQ(noise.sample1D(0.213f), smooth);

	const float oneOctave = noise.sample2D(0.317f, -0.729f);
	noise.setOctaves(5);
	noise.setPersistence(0.65f);
	noise.setLacunarity(2.3f);
	noise.setFrequency(0.37f);
	EXPECT_EQ(noise.octaves(), 5);
	EXPECT_FLOAT_EQ(noise.persistence(), 0.65f);
	EXPECT_FLOAT_EQ(noise.lacunarity(), 2.3f);
	EXPECT_FLOAT_EQ(noise.frequency(), 0.37f);
	EXPECT_NE(noise.sample2D(0.317f, -0.729f), oneOctave);

	const spk::Perlin unmodified(123u);
	EXPECT_NE(noise.sample3D(0.317f, -0.729f, 0.475f), unmodified.sample3D(0.317f, -0.729f, 0.475f));
}

TEST(PerlinTest, InvalidInputsAreRejectedWithoutCorruptingConfiguration)
{
	spk::Perlin noise(11u);
	const float before = noise.sample2D(0.375f, -2.125f);
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float infinity = std::numeric_limits<float>::infinity();

	expectInvalid([&]() {
		noise.setOctaves(0);
	});
	expectInvalid([&]() {
		noise.setOctaves(-1);
	});
	expectInvalid([&]() {
		noise.setOctaves(33);
	});
	expectInvalid([&]() {
		noise.setPersistence(-0.1f);
	});
	expectInvalid([&]() {
		noise.setPersistence(nan);
	});
	expectInvalid([&]() {
		noise.setLacunarity(0.0f);
	});
	expectInvalid([&]() {
		noise.setLacunarity(infinity);
	});
	expectInvalid([&]() {
		noise.setFrequency(0.0f);
	});
	expectInvalid([&]() {
		noise.setFrequency(nan);
	});
	expectInvalid([&]() {
		noise.setInterpolation(static_cast<spk::Perlin::Interpolation>(99));
	});
	expectInvalid([&]() {
		(void)noise.sample1D(nan);
	});
	expectInvalid([&]() {
		(void)noise.sample2D(0.0f, infinity);
	});
	expectInvalid([&]() {
		(void)noise.sample3D(0.0f, 0.0f, -infinity);
	});
	expectInvalid([&]() {
		(void)noise.sample1D(0.0f, 2.0f, 1.0f);
	});
	expectInvalid([&]() {
		(void)noise.sample2D(0.0f, 0.0f, 0.0f, nan);
	});
	expectInvalid([&]() {
		(void)noise.sample3D(0.0f, 0.0f, 0.0f, -infinity, 1.0f);
	});
	EXPECT_FLOAT_EQ(noise.sample2D(0.375f, -2.125f), before);
	EXPECT_EQ(noise.octaves(), 3);

	noise.setOctaves(2);
	noise.setFrequency(1.0e38f);
	noise.setLacunarity(100.0f);
	EXPECT_THROW((void)noise.sample1D(2.0f), std::out_of_range);
}

TEST(PerlinTest, ConstConcurrentSamplingMatchesSequentialSampling)
{
	const spk::Perlin noise(0xABCDEFu);
	constexpr std::size_t Samples = 512;
	constexpr std::size_t Workers = 4;
	std::array<float, Samples> reference{};
	for (std::size_t i = 0; i < Samples; ++i)
	{
		const float x = static_cast<float>(i) * 0.173f - 53.0f;
		reference[i] = noise.sample3D(x, x * 0.57f, -x * 0.29f);
	}

	std::array<std::array<float, Samples>, Workers> parallel{};
	std::vector<std::thread> workers;
	for (std::size_t thread = 0; thread < Workers; ++thread)
	{
		workers.emplace_back([&noise, &parallel, thread]() {
			for (std::size_t i = 0; i < Samples; ++i)
			{
				const float x = static_cast<float>(i) * 0.173f - 53.0f;
				parallel[thread][i] = noise.sample3D(x, x * 0.57f, -x * 0.29f);
			}
		});
	}
	for (auto &worker : workers)
	{
		worker.join();
	}
	for (const auto &sampleSet : parallel)
	{
		EXPECT_EQ(sampleSet, reference);
	}
}

TEST(PerlinPerformanceTest, ReportOneTwoAndThreeDimensionalSampleCosts)
{
	const spk::Perlin noise(123456u);
	constexpr int Samples = 20000;
	const auto benchmark = [&](const char *name, auto &&sample) {
		std::array<double, 5> durations{};
		double checksum = 0.0;
		for (double &duration : durations)
		{
			const auto start = std::chrono::steady_clock::now();
			for (int i = 0; i < Samples; ++i)
			{
				const float x = static_cast<float>(i) * 0.137f - 123.0f;
				checksum += sample(x);
			}
			duration = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count();
		}
		std::sort(durations.begin(), durations.end());
		std::cout << std::fixed << std::setprecision(2)
				  << "[Perlin performance] " << name << ": median "
				  << durations[2] / Samples << " ns/sample, checksum=" << checksum << '\n';
		EXPECT_TRUE(std::isfinite(checksum));
		EXPECT_GT(checksum, 0.0);
	};
	benchmark("1D", [&](float x) {
		return noise.sample1D(x);
	});
	benchmark("2D", [&](float x) {
		return noise.sample2D(x, x * 0.61f);
	});
	benchmark("3D", [&](float x) {
		return noise.sample3D(x, x * 0.61f, x * 0.13f);
	});
}

TEST(PerlinTest, QuinticInterpolationIsDistinctAndContinuous)
{
	spk::Perlin noise(118);
	noise.setOctaves(1);
	const float quintic = noise.sample2D(0.217f, -0.731f);
	noise.setInterpolation(spk::Perlin::Interpolation::SmoothStep);
	EXPECT_NE(noise.sample2D(0.217f, -0.731f), quintic);
	noise.setInterpolation(spk::Perlin::Interpolation::Quintic);
	EXPECT_FLOAT_EQ(noise.sample2D(0.217f, -0.731f), quintic);
	for (int i = -8; i <= 8; ++i)
	{
		const float x = static_cast<float>(i);
		constexpr float step = 0.002f;
		const float center = noise.sample2D(x, 0.371f);
		const float left = noise.sample2D(x - step, 0.371f);
		const float right = noise.sample2D(x + step, 0.371f);
		EXPECT_NEAR((center - left) / step, (right - center) / step, 0.01f);
	}
}

TEST(PerlinTest, LatticeGradientsUseDiverseDirectionsInTwoAndThreeDimensions)
{
	spk::Perlin noise(118);
	noise.setOctaves(1);
	std::set<std::array<int, 2>> directions2;
	std::set<std::array<int, 3>> directions3;
	constexpr float e = 0.002f;
	const auto quantize = [](float derivative) {
		return static_cast<int>(std::round(derivative * 20.0f));
	};
	for (int y = -8; y < 8; ++y)
	{
		for (int x = -8; x < 8; ++x)
		{
			const float a = static_cast<float>(x), b = static_cast<float>(y);
			const float z = static_cast<float>(x - y);
			directions2.insert({quantize((noise.sample2D(a + e, b) - noise.sample2D(a - e, b)) / (2 * e)), quantize((noise.sample2D(a, b + e) - noise.sample2D(a, b - e)) / (2 * e))});
			directions3.insert({quantize((noise.sample3D(a + e, b, z) - noise.sample3D(a - e, b, z)) / (2 * e)), quantize((noise.sample3D(a, b + e, z) - noise.sample3D(a, b - e, z)) / (2 * e)), quantize((noise.sample3D(a, b, z + e) - noise.sample3D(a, b, z - e)) / (2 * e))});
		}
	}
	EXPECT_GE(directions2.size(), 16u);
	EXPECT_GE(directions3.size(), 24u);
}

TEST(PerlinTest, ThresholdedLowFrequencyFieldAvoidsLongAxisAlignedArtifacts)
{
	spk::Perlin noise(118);
	noise.setFrequency(0.055f);
	noise.setOctaves(1);
	constexpr int Size = 80;
	std::array<std::array<bool, Size>, Size> field{};
	int filled = 0;
	for (int z = 0; z < Size; ++z)
	{
		for (int x = 0; x < Size; ++x)
		{
			field[z][x] = noise.sample2D(static_cast<float>(x - 32), static_cast<float>(z - 32)) >= 0.5f;
			filled += field[z][x];
		}
	}
	int longest = 0;
	for (int axis = 0; axis < Size - 1; ++axis)
	{
		int horizontal = 0, vertical = 0;
		for (int i = 0; i < Size; ++i)
		{
			horizontal = field[axis][i] != field[axis + 1][i] ? horizontal + 1 : 0;
			vertical = field[i][axis] != field[i][axis + 1] ? vertical + 1 : 0;
			longest = std::max({longest, horizontal, vertical});
		}
	}
	EXPECT_GT(filled, Size * Size / 4);
	EXPECT_LT(filled, Size * Size * 3 / 4);
	EXPECT_LT(longest, 33) << "Longest straight material boundary";
}

TEST(PerlinTest, DomainWarpIsOptionalBoundedAndDeterministic)
{
	spk::DomainWarp warp(718);
	const spk::Vector2 p{-12.3f, 5.7f};
	EXPECT_EQ(warp.warp2D(p), p);
	warp.setAmplitude(8);
	warp.xNoise().setFrequency(0.071f);
	warp.yNoise().setFrequency(0.083f);
	const auto first = warp.warp2D(p);
	EXPECT_NE(first, p);
	EXPECT_LE(std::abs(first.x - p.x), 8.001f);
	EXPECT_LE(std::abs(first.y - p.y), 8.001f);
	EXPECT_EQ(warp.warp2D(p), first);
	EXPECT_NE(warp.xNoise().seed(), warp.yNoise().seed());
	EXPECT_THROW(warp.setAmplitude(-1), std::invalid_argument);
	EXPECT_THROW((void)warp.warp2D({std::numeric_limits<float>::infinity(), 0}), std::invalid_argument);
	warp.setAmplitude(0);
	EXPECT_EQ(warp.warp2D(p), p);
}

TEST(PerlinTest, DomainWarpSupportsOneAndThreeDimensions)
{
	spk::DomainWarp warp(718);
	const float x = -12.3f;
	const spk::Vector3 p{x, 5.7f, -3.4f};
	EXPECT_FLOAT_EQ(warp.warp1D(x), x);
	EXPECT_EQ(warp.warp3D(p), p);
	warp.setAmplitude(8);
	warp.xNoise().setFrequency(0.071f);
	warp.yNoise().setFrequency(0.083f);
	warp.zNoise().setFrequency(0.097f);
	const auto one = warp.warp1D(x);
	const auto three = warp.warp3D(p);
	EXPECT_FLOAT_EQ(one, x + warp.xNoise().sample1D(x, -8, 8));
	EXPECT_FLOAT_EQ(three.x, p.x + warp.xNoise().sample3D(p.x, p.y, p.z, -8, 8));
	EXPECT_FLOAT_EQ(three.y, p.y + warp.yNoise().sample3D(p.x, p.y, p.z, -8, 8));
	EXPECT_FLOAT_EQ(three.z, p.z + warp.zNoise().sample3D(p.x, p.y, p.z, -8, 8));
	EXPECT_NE(three, p);
	EXPECT_LE(std::abs(one - x), 8.001f);
	EXPECT_LE(std::abs(three.x - p.x), 8.001f);
	EXPECT_LE(std::abs(three.y - p.y), 8.001f);
	EXPECT_LE(std::abs(three.z - p.z), 8.001f);
	EXPECT_EQ(warp.warp3D(p), three);
	EXPECT_NE(warp.xNoise().seed(), warp.zNoise().seed());
	EXPECT_NE(warp.yNoise().seed(), warp.zNoise().seed());
	const spk::Vector2 planar{x, p.y};
	const auto planarBefore = warp.warp2D(planar);
	warp.zNoise().setSeed(123);
	EXPECT_EQ(warp.warp2D(planar), planarBefore);
	EXPECT_FLOAT_EQ(warp.warp1D(x), one);
	EXPECT_FLOAT_EQ(warp.warp3D(p).x, three.x);
	EXPECT_FLOAT_EQ(warp.warp3D(p).y, three.y);
	EXPECT_THROW((void)warp.warp1D(std::numeric_limits<float>::quiet_NaN()), std::invalid_argument);
	EXPECT_THROW((void)warp.warp3D({0, 0, std::numeric_limits<float>::infinity()}), std::invalid_argument);
	warp.setAmplitude(0);
	EXPECT_FLOAT_EQ(warp.warp1D(x), x);
	EXPECT_EQ(warp.warp3D(p), p);
}
