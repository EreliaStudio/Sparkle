#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <future>
#include <limits>
#include <set>
#include <utility>
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
			soil.thickness = Stratum::Thickness::Fixed{2};
			soil.materials = {{2, 1}};
			strata().push_back(soil);
			Stratum deep;
			deep.thickness = Stratum::Thickness::Infinite{};
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
		std::atomic<const GenerationData *> _observedData{nullptr};

	protected:
		bool _prepareGenerationData(const spk::World::ChunkCoordinate &coordinate, GenerationData &data) override
		{
			const bool prepared = BiomeGenerator::_prepareGenerationData(coordinate, data);
			_observedData.store(&data);
			return prepared;
		}

		void _computeBiomePlacement(const spk::Vector2 &, Cell &cell) const override
		{
			++samples;
			cell[0] = {&_biome, 1};
		}

	public:
		mutable std::atomic<int> samples{0};

		[[nodiscard]] std::size_t activeChunkCount() const
		{
			return _observedData.load()->activeChunkCount();
		}

		[[nodiscard]] std::size_t columnCount() const
		{
			return _observedData.load()->columnCount();
		}

		Biome &biome()
		{
			return _biome;
		}

		explicit Generator(float scale = 1.0f) :
			BiomeGenerator(registry(), HeightCombination::Weighted, scale)
		{
			addPass<Strata>();
			addPass<Application>();
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
	ASSERT_TRUE(generator.generate({-2, 1, -1}).has_value());
	EXPECT_EQ(generator.samples.load(), 289);
	EXPECT_EQ(generator.activeChunkCount(), 0u);
	generator.flush();
	EXPECT_EQ(generator.columnCount(), 0u);
	ASSERT_TRUE(generator.generate({-2, 1, -1}).has_value());
	EXPECT_EQ(generator.samples.load(), 578);
}

TEST(VoxelBiomeGeneratorTest, StandardPipelineAcceptsSurfaceAndChunkPassesInOrder)
{
	using Data = Generator::GenerationData;
	using Coordinate = spk::World::ChunkCoordinate;

	class SurfaceProbe : public spk::World::SurfacePass<Chunk, Data>
	{
	private:
		std::vector<int> &_trace;

	protected:
		void _applySurface(const spk::Vector2Int &coordinate, Data &data) override
		{
			EXPECT_EQ(coordinate, (spk::Vector2Int{3, -2}));
			_trace.push_back(1);
			auto &column = data.column(coordinate);
			if (!column.appliedPasses.contains(this))
			{
				_trace.push_back(3);
				column.appliedPasses.insert(this);
			}
		}

	public:
		explicit SurfaceProbe(std::vector<int> &trace) :
			_trace(trace)
		{
		}
	};

	class ChunkProbe : public Generator::Pass
	{
	private:
		std::vector<int> &_trace;

	public:
		explicit ChunkProbe(std::vector<int> &trace) :
			_trace(trace)
		{
		}

		void apply(const Coordinate &coordinate, Data &) override
		{
			EXPECT_EQ(coordinate.x, 3);
			EXPECT_EQ(coordinate.z, -2);
			_trace.push_back(2);
		}
	};

	Generator generator;
	std::vector<int> trace;
	generator.addPass<SurfaceProbe>(trace);
	generator.addPass<ChunkProbe>(trace);
	ASSERT_TRUE(generator.generate({3, 0, -2}));
	ASSERT_TRUE(generator.generate({3, 1, -2}));
	EXPECT_EQ(trace, (std::vector<int>{1, 3, 2, 1, 2}));
	generator.flush();
	ASSERT_TRUE(generator.generate({3, 2, -2}));
	EXPECT_EQ(trace, (std::vector<int>{1, 3, 2, 1, 2, 1, 3, 2}));
}

TEST(VoxelBiomeGeneratorTest, FailedSurfacePassDoesNotPublishPartialColumn)
{
	class FailingPass : public Generator::SurfacePass
	{
	private:
		bool _first = true;

	protected:
		void _applySurface(const spk::Vector2Int &coordinate, Generator::GenerationData &data) override
		{
			auto &column = data.column(coordinate);
			if (column.appliedPasses.contains(this))
			{
				return;
			}
			auto &height = column.heights->sample({0, 0});
			EXPECT_FLOAT_EQ(height, 17.75f);
			if (std::exchange(_first, false))
			{
				height += 1.0f;
				throw spk::Exception("Injected surface failure");
			}
			column.appliedPasses.insert(this);
		}
	};

	Generator generator;
	generator.addPass<FailingPass>();
	EXPECT_THROW((void)generator.generate({0, 0, 0}), spk::Exception);
	EXPECT_EQ(generator.activeChunkCount(), 0u);
	EXPECT_EQ(generator.samples.load(), 289);
	ASSERT_TRUE(generator.generate({0, 0, 0}));
	EXPECT_EQ(generator.samples.load(), 578);
	ASSERT_TRUE(generator.generate({0, 1, 0}));
	EXPECT_EQ(generator.samples.load(), 578);
}

TEST(VoxelBiomeGeneratorTest, FiniteStrataAndCustomMaterialSelection)
{
	Biome biome;
	biome.strata().resize(1);
	auto &stratum = biome.strata().front();
	stratum.thickness = Biome::Stratum::Thickness::Fixed{2};
	stratum.materials = {{1, 1}, {4, 3}};
	stratum.distribution = Biome::Stratum::Distribution::Custom{[](const spk::Vector2 &position) {
		return position.x < 0 ? 0.0f : 1.0f;
	}};
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
		stratum.distribution = Stratum::Distribution::Perlin{};
		std::get<Stratum::Distribution::Perlin>(stratum.distribution).noise.setSeed(seed);
		std::get<Stratum::Distribution::Perlin>(stratum.distribution).noise.setFrequency(frequency);
		std::get<Stratum::Distribution::Perlin>(stratum.distribution).noise.setOctaves(1);
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
			const double value = std::get<Stratum::Distribution::Perlin>(stratum.distribution).noise.sample2D(p.x, p.y);
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
	stratum.distribution = Stratum::Distribution::Weighted{928};
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
	std::get<Stratum::Distribution::Weighted>(stratum.distribution).seed++;
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
	stratum.thickness = Stratum::Thickness::Variable{2, 13, spk::Perlin{}};
	std::get<Stratum::Thickness::Variable>(stratum.thickness).noise.setSeed(31);
	std::get<Stratum::Thickness::Variable>(stratum.thickness).noise.setFrequency(0.069f);
	configurePerlin(stratum, 721, 0.19f);
	std::set<std::uint32_t> thicknesses;
	for (int z = -15; z <= 15; ++z)
	{
		for (int x = -15; x <= 15; ++x)
		{
			const spk::Vector2 p{static_cast<float>(x), static_cast<float>(z)};
			const auto result = sampled(biome, p).front();
			const double noise = std::get<Stratum::Thickness::Variable>(stratum.thickness).noise.sample2D(p.x, p.y);
			const auto expected = 2u + static_cast<std::uint32_t>(std::min(11.0, std::floor(noise * 12)));
			ASSERT_TRUE(result.thickness.has_value());
			EXPECT_EQ(*result.thickness, expected);
			thicknesses.insert(*result.thickness);
			const auto id = std::get<Stratum::Distribution::Perlin>(stratum.distribution).noise.sample2D(p.x, p.y) < 0.5f ? 1u : 2u;
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
	strata[1].thickness = Stratum::Thickness::Variable{1, 9, spk::Perlin{}};
	std::get<Stratum::Thickness::Variable>(strata[1].thickness).noise.setSeed(65);
	std::get<Stratum::Thickness::Variable>(strata[1].thickness).noise.setFrequency(0.077f);
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
	generator.flush();
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
	s.thickness = Stratum::Thickness::Fixed{0};
	EXPECT_THROW((void)sample(), spk::Exception);
	s.thickness = Stratum::Thickness::Variable{4, 3, spk::Perlin{}};
	EXPECT_THROW((void)sample(), spk::Exception);
	s.thickness = Stratum::Thickness::Fixed{1};
	s.distribution = Stratum::Distribution::Custom{};
	EXPECT_THROW((void)sample(), spk::Exception);
	for (float value : {-0.01f, 1.01f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
	{
		s.distribution = Stratum::Distribution::Custom{[value](const spk::Vector2 &) {
			return value;
		}};
		EXPECT_THROW((void)sample(), spk::Exception);
	}
	s.distribution = Stratum::Distribution::Weighted{};
	s.thickness = Stratum::Thickness::Infinite{};
	biome.strata().push_back(Stratum{});
	EXPECT_THROW((void)sample(), spk::Exception);
}

TEST(VoxelBiomeGeneratorTest, DifferentGridScalesUseExpectedSampleCounts)
{
	for (const auto &[scale, expected] : std::array<std::pair<float, int>, 4>{{{0.5f, 1089}, {1, 289}, {2, 81}, {4, 25}}})
	{
		Generator generator(scale);
		const auto upper = generator.generate({-1, 1, 1});
		const auto lower = generator.generate({-1, 0, 1});
		ASSERT_TRUE(upper && lower);
		EXPECT_EQ(generator.samples.load(), expected);
		EXPECT_EQ(idAt(*upper, 15, 1, 15), 1u);
		EXPECT_EQ(idAt(*lower, 15, 15, 15), 2u);
	}
	EXPECT_THROW((void)Generator(3.0f), spk::Exception);
}

TEST(VoxelBiomeGeneratorTest, LocalRequesterReusesCachedColumnWithoutObserver)
{
	Generator generator;
	spk::WorkerPool workers(1);
	spk::World::LocalChunkRequester<Chunk, Generator::GenerationData> requester(workers, generator);
	ASSERT_TRUE(generator.generate({0, 0, 0}));
	std::promise<void> unblock;
	auto gate = unblock.get_future().share();
	auto blocker = workers.submit([gate] {
		gate.wait();
		return 0;
	});
	auto upper = requester.request({0, 1, 0});
	unblock.set_value();
	EXPECT_EQ(blocker.get(), 0);
	ASSERT_TRUE(upper.get());
	EXPECT_EQ(generator.samples.load(), 289);
	ASSERT_TRUE(generator.generate({0, 0, 0}));
	EXPECT_EQ(generator.samples.load(), 289);
	EXPECT_EQ(generator.activeChunkCount(), 0u);
	generator.flush();
	ASSERT_TRUE(generator.generate({0, 0, 0}));
	EXPECT_EQ(generator.samples.load(), 578);
}

TEST(VoxelBiomeGeneratorTest, SimultaneousDuplicateCoordinatesAreRejectedWithoutErasingActiveScratch)
{
	class BlockingPass : public Generator::Pass
	{
	private:
		std::promise<void> &_entered;
		std::shared_future<void> _release;
		std::atomic<bool> _blocked{false};

	public:
		BlockingPass(std::promise<void> &entered, std::shared_future<void> release) :
			_entered(entered),
			_release(std::move(release))
		{
		}

		void apply(const spk::World::ChunkCoordinate &, Generator::GenerationData &) override
		{
			if (!_blocked.exchange(true))
			{
				_entered.set_value();
				_release.wait();
			}
		}
	};

	std::promise<void> entered;
	std::promise<void> release;
	Generator generator;
	generator.addPass<BlockingPass>(entered, release.get_future().share());
	auto first = std::async(std::launch::async, [&] {
		return generator.generate({0, 0, 0});
	});
	entered.get_future().wait();
	EXPECT_EQ(generator.activeChunkCount(), 1u);
	EXPECT_THROW((void)generator.generate({0, 0, 0}), spk::Exception);
	EXPECT_EQ(generator.activeChunkCount(), 1u);
	release.set_value();
	const auto a = first.get();
	ASSERT_TRUE(a);
	EXPECT_EQ(generator.activeChunkCount(), 0u);
	const auto b = generator.generate({0, 0, 0});
	ASSERT_TRUE(b);
	EXPECT_EQ(idAt(*a, 0, 15, 0), idAt(*b, 0, 15, 0));
	EXPECT_EQ(generator.samples.load(), 289);
	EXPECT_EQ(generator.activeChunkCount(), 0u);
}
