#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <future>
#include <limits>
#include <set>
#include <vector>

#include "voxel/biome_generator.hpp"
#include "voxel/chunk.hpp"
#include "world/local_chunk_requester.hpp"

namespace
{
	using Chunk = spk::Voxel::Chunk<16, 16, 16>;

	spk::Voxel::Definition::Registry registry()
	{
		spk::Voxel::Shape::Builder shape;
		shape.addSlot("surface");
		const auto definition = spk::Voxel::Definition(shape.build(), {{"surface", {"terrain"}}});
		spk::Voxel::Definition::Registry::Builder builder;
		for (std::uint32_t id = 1; id <= 4; ++id)
		{
			builder.add(id, definition);
		}
		return builder.build();
	}

	class Biome : public spk::Voxel::Biome
	{
	public:
		Biome()
		{
			Stratum surface;
			surface.materials = {{1, 1}};
			strata().push_back(surface);
			Stratum soil;
			soil.minimumThickness = 2;
			soil.maximumThickness = 2;
			soil.materials = {{2, 1}};
			strata().push_back(soil);
			Stratum deep;
			deep.thicknessMode = Stratum::ThicknessMode::Infinite;
			deep.materials = {{3, 1}};
			strata().push_back(deep);
		}

		float groundHeight(const spk::Vector2 &) const override
		{
			return 17.75f;
		}
	};

	class Generator : public spk::Voxel::BiomeGenerator<Chunk, 1>
	{
	private:
		Biome _biome;

	protected:
		void _computeBiomePlacement(const spk::Vector2 &, Cell &cell) const override
		{
			++samples;
			cell[0] = {&_biome, 1};
		}

	public:
		mutable std::atomic<int> samples{0};
		Biome &biome()
		{
			return _biome;
		}

		explicit Generator(float scale = 1.0f) :
			BiomeGenerator(registry(), HeightCombination::Weighted, scale)
		{
		}
	};

	std::uint32_t idAt(const Chunk &chunk, std::uint32_t x, std::uint32_t y, std::uint32_t z)
	{
		return spk::Voxel::Volume::PackedCell::definitionId(chunk.volume().atRaw({x, y, z}));
	}
}

TEST(VoxelBiomeGeneratorTest, VerticalStackSharesDistributionAndAppliesStrata)
{
	Generator generator;
	generator.retain({-2, 0, -1});
	generator.retain({-2, 1, -1});
	auto first = std::async(std::launch::async, [&] {
		return generator.generate({-2, 0, -1});
	});
	auto second = std::async(std::launch::async, [&] {
		return generator.generate({-2, 1, -1});
	});
	const auto lower = first.get();
	const auto upper = second.get();
	ASSERT_TRUE(lower.has_value());
	ASSERT_TRUE(upper.has_value());
	EXPECT_EQ(generator.samples.load(), 289);
	EXPECT_EQ(idAt(*lower, 0, 14, 0), 3u);
	EXPECT_EQ(idAt(*lower, 0, 15, 0), 2u);
	EXPECT_EQ(idAt(*upper, 0, 0, 0), 2u);
	EXPECT_EQ(idAt(*upper, 0, 1, 0), 1u);
	EXPECT_EQ(idAt(*upper, 0, 2, 0), 0u);
	generator.release({-2, 0, -1});
	generator.release({-2, 1, -1});
	ASSERT_TRUE(generator.generate({-2, 1, -1}).has_value());
	EXPECT_EQ(generator.samples.load(), 578);
}

TEST(VoxelBiomeGeneratorTest, FiniteStrataAndCustomMaterialSelection)
{
	Biome biome;
	biome.strata().resize(1);
	auto &stratum = biome.strata().front();
	stratum.minimumThickness = 2;
	stratum.maximumThickness = 2;
	stratum.materials = {{1, 1}, {4, 3}};
	stratum.distribution = Biome::Stratum::Distribution::Custom;
	stratum.customDistribution = [](const spk::Vector2 &position) {
		return position.x < 0 ? 0.0f : 1.0f;
	};
	biome.sampleStrata({-1, 0}, [](const auto &layer) {
		EXPECT_EQ(layer.voxelID, 1u);
		EXPECT_EQ(layer.thickness, 2u);
	});
	biome.sampleStrata({1, 0}, [](const auto &layer) {
		EXPECT_EQ(layer.voxelID, 4u);
		EXPECT_EQ(layer.thickness, 2u);
	});
	stratum.materials[0].voxelID = 0;
	EXPECT_THROW(biome.sampleStrata({0, 0}, [](const auto &) {
	}),
				 spk::Exception);
}

namespace
{
	using Stratum = Biome::Stratum;
	using Layer = Biome::Layer;

	std::vector<Layer> sampled(const Biome &biome, const spk::Vector2 &position)
	{
		std::vector<Layer> layers;
		biome.sampleStrata(position, [&](Layer layer) {
			layers.push_back(layer);
		});
		return layers;
	}

	void configurePerlin(Stratum &stratum, std::uint32_t seed, float frequency)
	{
		stratum.distribution = Stratum::Distribution::Perlin;
		stratum.materialNoise.setSeed(seed);
		stratum.materialNoise.setFrequency(frequency);
		stratum.materialNoise.setOctaves(1);
	}
}

TEST(VoxelBiomeGeneratorTest, PerlinMaterialSelectionMatchesWeightedThresholds)
{
	Biome biome;
	biome.strata().resize(1);
	auto &stratum = biome.strata().front();
	stratum.materials = {{1, 2}, {2, 1}, {3, 2}, {4, 0}};
	configurePerlin(stratum, 437, 0.073f);
	std::set<std::uint32_t> materials;
	for (int z = -32; z < 32; ++z)
	{
		for (int x = -32; x < 32; ++x)
		{
			const spk::Vector2 p{static_cast<float>(x), static_cast<float>(z)};
			const double value = stratum.materialNoise.sample2D(p.x, p.y);
			const auto expected = value * 5 < 2 ? 1u : value * 5 < 3 ? 2u
																	 : 3u;
			const auto result = sampled(biome, p);
			ASSERT_EQ(result.size(), 1u);
			EXPECT_EQ(result.front().voxelID, expected);
			materials.insert(result.front().voxelID);
		}
	}
	EXPECT_EQ(materials, (std::set<std::uint32_t>{1, 2, 3}));
}

TEST(VoxelBiomeGeneratorTest, WeightedDistributionIsDeterministicAndHonorsZeroWeights)
{
	Biome biome;
	biome.strata().resize(1);
	auto &stratum = biome.strata().front();
	stratum.materials = {{1, 3}, {2, 1}, {4, 0}};
	stratum.seed = 928;
	std::vector<std::uint32_t> expected;
	int count = 0;
	for (int i = -2048; i < 2048; ++i)
	{
		const auto result = sampled(biome, {static_cast<float>(i), -17});
		const auto id = result.front().voxelID;
		ASSERT_TRUE(id == 1 || id == 2);
		count += id == 2;
		expected.push_back(id);
	}
	EXPECT_GT(count, 800);
	EXPECT_LT(count, 1250);
	for (int i = 2047; i >= -2048; --i)
	{
		EXPECT_EQ(sampled(biome, {static_cast<float>(i), -17}).front().voxelID, expected[static_cast<std::size_t>(i + 2048)]);
	}
	stratum.seed++;
	bool changed = false;
	for (int i = -128; i < 128; ++i)
	{
		changed |= sampled(biome, {static_cast<float>(i), -17}).front().voxelID != expected[static_cast<std::size_t>(i + 2048)];
	}
	EXPECT_TRUE(changed);
}

TEST(VoxelBiomeGeneratorTest, ThicknessNoiseIsIndependentOfMaterialNoise)
{
	Biome biome;
	biome.strata().resize(1);
	auto &stratum = biome.strata().front();
	stratum.materials = {{1, 1}, {2, 1}};
	stratum.minimumThickness = 2;
	stratum.maximumThickness = 13;
	stratum.thicknessNoise.setSeed(31);
	stratum.thicknessNoise.setFrequency(0.069f);
	configurePerlin(stratum, 721, 0.19f);
	std::set<std::uint32_t> thicknesses;
	for (int z = -15; z <= 15; ++z)
	{
		for (int x = -15; x <= 15; ++x)
		{
			const spk::Vector2 p{static_cast<float>(x), static_cast<float>(z)};
			const auto result = sampled(biome, p).front();
			const double noise = stratum.thicknessNoise.sample2D(p.x, p.y);
			const auto expected = 2u + static_cast<std::uint32_t>(std::min(11.0, std::floor(noise * 12)));
			ASSERT_TRUE(result.thickness.has_value());
			EXPECT_EQ(*result.thickness, expected);
			thicknesses.insert(*result.thickness);
			const auto id = stratum.materialNoise.sample2D(p.x, p.y) < 0.5f ? 1u : 2u;
			EXPECT_EQ(result.voxelID, id);
		}
	}
	EXPECT_GT(thicknesses.size(), 2u);
}

TEST(VoxelBiomeGeneratorTest, PerlinStrataAreAppliedToEveryVoxelAcrossVerticalAndHorizontalChunks)
{
	Generator generator;
	auto &biome = generator.biome();
	auto &strata = biome.strata();
	strata[1].minimumThickness = 1;
	strata[1].maximumThickness = 9;
	strata[1].thicknessNoise.setSeed(65);
	strata[1].thicknessNoise.setFrequency(0.077f);
	strata[1].materials = {{2, 1}, {4, 1}};
	configurePerlin(strata[1], 33, 0.12f);
	strata[2].materials = {{3, 1}, {4, 1}};
	configurePerlin(strata[2], 123, 0.09f);
	for (int cz : {-1, 0})
	{
		for (int cx : {-1, 0})
		{
			for (int cy : {1, -1, 0})
			{
				const auto chunk = generator.generate({cx, cy, cz});
				ASSERT_TRUE(chunk.has_value());
				for (std::uint32_t z = 0; z < 16; ++z)
				{
					for (std::uint32_t x = 0; x < 16; ++x)
					{
						const spk::Vector2 p{static_cast<float>(cx * 16 + static_cast<int>(x)), static_cast<float>(cz * 16 + static_cast<int>(z))};
						const auto layers = sampled(biome, p);
						for (std::uint32_t y = 0; y < 16; ++y)
						{
							int depth = 17 - (cy * 16 + static_cast<int>(y));
							std::uint32_t expected = 0;
							if (depth >= 0)
							{
								for (const auto &layer : layers)
								{
									if (!layer.thickness || depth < static_cast<int>(*layer.thickness))
									{
										expected = layer.voxelID;
										break;
									}
									depth -= static_cast<int>(*layer.thickness);
								}
							}
							ASSERT_EQ(idAt(*chunk, x, y, z), expected) << cx << "," << cy << "," << cz << " local " << x << "," << y << "," << z;
						}
					}
				}
			}
		}
	}
}

TEST(VoxelBiomeGeneratorTest, FiniteAndEmptyStrataLeaveUnfilledVoxels)
{
	Generator generator;
	generator.biome().strata().resize(2);
	const auto lower = generator.generate({0, 0, 0});
	const auto upper = generator.generate({0, 1, 0});
	ASSERT_TRUE(lower && upper);
	EXPECT_EQ(idAt(*lower, 0, 14, 0), 0u);
	EXPECT_EQ(idAt(*lower, 0, 15, 0), 2u);
	EXPECT_EQ(idAt(*upper, 0, 0, 0), 2u);
	EXPECT_EQ(idAt(*upper, 0, 1, 0), 1u);
	EXPECT_EQ(idAt(*upper, 0, 2, 0), 0u);
	generator.biome().strata().clear();
	const auto empty = generator.generate({0, 1, 0});
	ASSERT_TRUE(empty);
	for (std::uint32_t y = 0; y < 16; ++y)
	{
		EXPECT_EQ(idAt(*empty, 0, y, 0), 0u);
	}
}

TEST(VoxelBiomeGeneratorTest, InfiniteStratumCoversArbitrarilyDeepChunks)
{
	Generator generator;
	auto &strata = generator.biome().strata();
	strata.erase(strata.begin(), strata.end() - 1);
	const auto deep = generator.generate({-8, -300, 7});
	ASSERT_TRUE(deep);
	EXPECT_EQ(idAt(*deep, 0, 0, 0), 3u);
	EXPECT_EQ(idAt(*deep, 15, 15, 15), 3u);
}

TEST(VoxelBiomeGeneratorTest, RejectsInvalidThicknessAndMaterialConfigurations)
{
	Biome biome;
	biome.strata().resize(1);
	auto &s = biome.strata().front();
	const auto sample = [&] {
		return sampled(biome, {0, 0});
	};
	s.materials.clear();
	EXPECT_THROW((void)sample(), spk::Exception);
	s.materials = {{1, 0}};
	EXPECT_THROW((void)sample(), spk::Exception);
	s.materials = {{1, -1}};
	EXPECT_THROW((void)sample(), spk::Exception);
	s.materials = {{1, std::numeric_limits<float>::quiet_NaN()}};
	EXPECT_THROW((void)sample(), spk::Exception);
	s.materials = {{spk::Voxel::Volume::PackedCell::MaxDefinitionId + 1u, 1}};
	EXPECT_THROW((void)sample(), spk::Exception);
	s.materials = {{1, 1}};
	s.minimumThickness = 0;
	EXPECT_THROW((void)sample(), spk::Exception);
	s.minimumThickness = 4;
	s.maximumThickness = 3;
	EXPECT_THROW((void)sample(), spk::Exception);
	s.minimumThickness = s.maximumThickness = 1;
	s.distribution = Stratum::Distribution::Custom;
	EXPECT_THROW((void)sample(), spk::Exception);
	for (float value : {-0.01f, 1.01f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
	{
		s.customDistribution = [value](const spk::Vector2 &) {
			return value;
		};
		EXPECT_THROW((void)sample(), spk::Exception);
	}
	s.distribution = Stratum::Distribution::Weighted;
	s.thicknessMode = Stratum::ThicknessMode::Infinite;
	biome.strata().push_back(Stratum{});
	EXPECT_THROW((void)sample(), spk::Exception);
}

TEST(VoxelBiomeGeneratorTest, DifferentGridScalesUseExpectedSampleCounts)
{
	for (const auto &[scale, expected] : std::array<std::pair<float, int>, 4>{{{0.5f, 1089}, {1, 289}, {2, 81}, {4, 25}}})
	{
		Generator generator(scale);
		generator.retain({-1, 0, 1});
		generator.retain({-1, 1, 1});
		const auto upper = generator.generate({-1, 1, 1});
		const auto lower = generator.generate({-1, 0, 1});
		ASSERT_TRUE(upper && lower);
		EXPECT_EQ(generator.samples.load(), expected);
		EXPECT_EQ(idAt(*upper, 15, 1, 15), 1u);
		EXPECT_EQ(idAt(*lower, 15, 15, 15), 2u);
		generator.release({-1, 0, 1});
		generator.release({-1, 1, 1});
	}
	EXPECT_THROW((void)Generator(3.0f), spk::Exception);
}

TEST(VoxelBiomeGeneratorTest, LocalRequesterRetainsQueuedColumnAfterObserverRelease)
{
	Generator generator;
	spk::WorkerPool workers(1);
	spk::World::LocalChunkRequester<Chunk, Generator::GenerationData, Generator::Builder> requester(workers, generator);
	requester.retain({0, 0, 0});
	ASSERT_TRUE(generator.generate({0, 0, 0}));
	std::promise<void> unblock;
	auto gate = unblock.get_future().share();
	auto blocker = workers.submit([gate] {
		gate.wait();
		return 0;
	});
	auto upper = requester.request({0, 1, 0});
	requester.release({0, 0, 0});
	unblock.set_value();
	EXPECT_EQ(blocker.get(), 0);
	ASSERT_TRUE(upper.get());
	EXPECT_EQ(generator.samples.load(), 289);
	ASSERT_TRUE(generator.generate({0, 0, 0}));
	EXPECT_EQ(generator.samples.load(), 578);
}
