#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <future>
#include <limits>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "world/local_chunk_requester.hpp"
#include "world/voxel/biome_generator.hpp"
#include "world/voxel/chunk.hpp"

namespace
{
	using Chunk = spk::World::Voxel::Chunk<16, 16, 16>;

	using Influence = spk::World::SurfaceBiome::Influence;
	using DefaultGenerationData = spk::World::Voxel::Biome::Generator<Chunk>::Data;
	static_assert(std::is_same_v<Influence::Cell<>, Influence::Cell<4>>);
	static_assert(std::is_same_v<Influence::Grid<>, Influence::Grid<4>>);
	static_assert(std::is_same_v<DefaultGenerationData, spk::World::Voxel::Biome::Generator<Chunk, 4>::Data>);
	static_assert(std::is_same_v<DefaultGenerationData, spk::World::Voxel::Biome::GenerationData<4>>);
	static_assert(std::is_same_v<decltype(std::declval<const DefaultGenerationData &>().world()), const DefaultGenerationData::World &>);
	static_assert(std::is_same_v<decltype(std::declval<DefaultGenerationData &>().column(spk::Vector2Int{})), DefaultGenerationData::Column &>);
	static_assert(std::is_same_v<decltype(std::declval<DefaultGenerationData &>().chunk(spk::World::ChunkCoordinate{})), DefaultGenerationData::Chunk &>);
	static_assert(std::is_same_v<spk::World::Voxel::Biome::Generator<Chunk>, spk::World::Voxel::Biome::Generator<Chunk, 4>>);
	static_assert(std::is_same_v<spk::World::SurfaceBiomeDistributionPass<Chunk, DefaultGenerationData>, spk::World::SurfaceBiomeDistributionPass<Chunk, DefaultGenerationData, 4>>);
	static_assert(std::is_same_v<spk::World::SurfaceHeightCombinationPass<Chunk, DefaultGenerationData>, spk::World::SurfaceHeightCombinationPass<Chunk, DefaultGenerationData, 4>>);
	static_assert(std::is_same_v<spk::World::Voxel::Strata::GenerationPass<Chunk, DefaultGenerationData>, spk::World::Voxel::Strata::GenerationPass<Chunk, DefaultGenerationData, 4>>);

	const spk::Voxel::Definition::Registry &registry()
	{
		static const auto value = []() {
			spk::Voxel::Shape::Builder shape;
			shape.addSlot("surface");
			const auto definition = spk::Voxel::Definition(shape.build(), {{"surface", {"terrain"}}});
			spk::Voxel::Definition::Registry::Builder builder;
			for (std::uint32_t id = 1; id <= 4; ++id)
			{
				builder.add(id, definition);
			}
			return builder.build();
		}();
		return value;
	}

	class Biome : public spk::World::Voxel::Biome
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

	class SingleBiomePlacement
	{
	public:
		Biome biome;
		mutable std::atomic<int> samples{0};

		void compute(const spk::Vector2 &, spk::World::SurfaceBiome::Influence::Cell<1> &cell) const
		{
			++samples;
			cell[0] = {&biome, 1};
		}
	};

	class Generator : public spk::World::Voxel::Biome::Generator<Chunk, 1>
	{
	private:
		class SingleBiomeDistributionPass final : public DistributionPass
		{
		private:
			SingleBiomePlacement _placement;
			Distribution _function = [this](const spk::Vector2 &position, Cell &cell) {
				_placement.compute(position, cell);
			};

		public:
			explicit SingleBiomeDistributionPass(float scale)
			{
				setScale(scale);
				bind(&_function);
			}

			SingleBiomePlacement &placement() noexcept
			{
				return _placement;
			}
		};

		SingleBiomeDistributionPass &_distributionPass;
		std::atomic<const Data *> _observedData{nullptr};

	protected:
		bool _prepareGenerationData(const spk::World::ChunkCoordinate &coordinate, Data &data) override
		{
			const bool prepared = spk::World::Voxel::Biome::Generator<Chunk, 1>::_prepareGenerationData(coordinate, data);
			_observedData.store(&data);
			return prepared;
		}

	public:
		std::atomic<int> &samples;

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
			return _distributionPass.placement().biome;
		}

		explicit Generator(float scale = 1.0f) :
			_distributionPass(addPass<SingleBiomeDistributionPass>(scale)),
			samples(_distributionPass.placement().samples)
		{
			bind(&registry());
			addPass<CombinationPass>(HeightCombination::Weighted);
			addPass<StrataGeneration>();
			addPass<Application>();
		}
	};

	class AlternateBiome final : public Biome
	{
	public:
		AlternateBiome()
		{
			strata().front().materials = {{4, 1}};
		}

		float groundHeight(const spk::Vector2 &) const override
		{
			return 5.75f;
		}
	};

	class AlternateGenerator final : public spk::World::Voxel::Biome::Generator<Chunk, 1>
	{
	private:
		class AlternateDistributionPass final : public DistributionPass
		{
		private:
			AlternateBiome _biome;
			Distribution _function = [this](const spk::Vector2 &, Cell &cell) {
				cell[0] = {&_biome, 1.0f};
			};

		public:
			AlternateDistributionPass()
			{
				bind(&_function);
			}
		};

	public:
		AlternateGenerator()
		{
			addPass<AlternateDistributionPass>();
			addPass<CombinationPass>(HeightCombination::Dominant);
			addPass<StrataGeneration>();
			addPass<Application>();
		}
	};

	class MixedGenerator final : public spk::World::Voxel::Biome::Generator<Chunk, 2>
	{
	private:
		class MixedDistributionPass final : public DistributionPass
		{
		private:
			Biome _first;
			Biome _second;
			Distribution _function = [this](const spk::Vector2 &, Cell &cell) {
				cell[0] = {&_first, 0.8f};
				cell[1] = {&_second, 0.2f};
			};

		public:
			MixedDistributionPass()
			{
				for (auto &stratum : _first.strata())
				{
					stratum.materials = {{1, 1}};
				}
				for (auto &stratum : _second.strata())
				{
					stratum.materials = {{4, 1}};
				}
				bind(&_function);
			}
		};

	public:
		explicit MixedGenerator(BiomeSelection selection, std::uint32_t seed)
		{
			bind(&registry());
			addPass<MixedDistributionPass>();
			addPass<CombinationPass>(HeightCombination::Weighted);
			addPass<StrataGeneration>(selection, seed);
			addPass<Application>();
		}
	};

	std::uint32_t idAt(const Chunk &chunk, std::uint32_t x, std::uint32_t y, std::uint32_t z)
	{
		return spk::Voxel::Volume::PackedCell::definitionId(chunk.volume().atRaw({x, y, z}));
	}
}

TEST(VoxelBiomeGeneratorTest, StrataApplicationSupportsIndependentGenerationData)
{
	using Strata = spk::World::Voxel::Strata;
	using Coordinate = spk::World::ChunkCoordinate;

	struct Data
	{
		spk::Voxel::Volume::Builder volume{spk::Vector3UInt{Chunk::Size}};
		Strata::Grid::Lease profiles = Strata::Grid::obtain({16, 16});

		void flush()
		{
		}
	};

	class Pass : public Strata::ApplicationPass<Chunk, Data>
	{
	protected:
		spk::Voxel::Volume::Builder &_builder(const Coordinate &coordinate, Data &data) override
		{
			EXPECT_EQ(coordinate, (Coordinate{0, 1, 0}));
			return data.volume;
		}

		const Strata::Grid &_strataGrid(const Coordinate &coordinate, const Data &data) override
		{
			EXPECT_EQ(coordinate, (Coordinate{0, 1, 0}));
			return *data.profiles;
		}
	};

	Data data;
	data.volume.bind(registry());
	data.volume.set({0, 2, 0}, 4);
	Biome biome;
	for (auto &&[position, instance] : *data.profiles)
	{
		instance = Strata::Instance::obtain(biome, {static_cast<float>(position.x), static_cast<float>(position.y)}, 17.75);
	}

	Pass pass;
	pass.apply({0, 1, 0}, data);
	Chunk chunk;
	chunk.setVolume(data.volume.build());
	EXPECT_EQ(idAt(chunk, 0, 0, 0), 2u);
	EXPECT_EQ(idAt(chunk, 0, 1, 0), 1u);
	EXPECT_EQ(idAt(chunk, 0, 2, 0), 4u);
	EXPECT_EQ(idAt(chunk, 0, 3, 0), 0u);
}

TEST(VoxelBiomeGeneratorTest, PlacementIsConfiguredByConcreteRecipe)
{
	static_assert(!std::is_default_constructible_v<spk::World::Voxel::Biome::Generator<Chunk, 1>>);

	Generator generator;
	ASSERT_TRUE(generator.generate({0, 0, 0}));
	EXPECT_EQ(generator.samples.load(), 289);
	ASSERT_TRUE(generator.generate({0, 1, 0}));
	EXPECT_EQ(generator.samples.load(), 289);
}

TEST(VoxelBiomeGeneratorTest, SeparateRecipesOwnTheirDistributionAndHeightMode)
{
	Generator first;
	AlternateGenerator second;
	second.bind(&registry());

	const auto firstChunk = first.generate({0, 0, 0});
	const auto secondChunk = second.generate({0, 0, 0});
	ASSERT_TRUE(firstChunk);
	ASSERT_TRUE(secondChunk);
	EXPECT_EQ(idAt(*firstChunk, 0, 15, 0), 2u);
	EXPECT_EQ(idAt(*secondChunk, 0, 5, 0), 4u);
	EXPECT_EQ(idAt(*secondChunk, 0, 6, 0), 0u);

	const auto firstUpper = first.generate({0, 1, 0});
	const auto secondUpper = second.generate({0, 1, 0});
	ASSERT_TRUE(firstUpper);
	ASSERT_TRUE(secondUpper);
	EXPECT_EQ(idAt(*firstUpper, 0, 1, 0), 1u);
	EXPECT_EQ(idAt(*secondUpper, 0, 1, 0), 0u);
	EXPECT_EQ(first.samples.load(), 289);
}

TEST(VoxelBiomeGeneratorTest, WeightedBiomeSelectionIsSeededAndSharedByVerticalStrata)
{
	using Selection = MixedGenerator::BiomeSelection;
	MixedGenerator dominant(Selection::Dominant, 42);
	MixedGenerator weighted(Selection::Weighted, 42);
	MixedGenerator sameSeed(Selection::Weighted, 42);
	MixedGenerator otherSeed(Selection::Weighted, 43);
	std::size_t firstCount = 0;
	std::size_t changedCount = 0;
	for (const int cx : {-2, -1, 0, 1})
	{
		const auto d = dominant.generate({cx, 1, -1});
		const auto upper = weighted.generate({cx, 1, -1});
		const auto lower = weighted.generate({cx, 0, -1});
		const auto other = otherSeed.generate({cx, 1, -1});
		ASSERT_TRUE(d && upper && lower && other);
		for (std::uint32_t z = 0; z < 16; ++z)
		{
			for (std::uint32_t x = 0; x < 16; ++x)
			{
				const auto id = idAt(*upper, x, 1, z);
				EXPECT_EQ(idAt(*d, x, 1, z), 1u);
				EXPECT_TRUE(id == 1u || id == 4u);
				EXPECT_EQ(idAt(*lower, x, 0, z), id);
				EXPECT_EQ(idAt(*upper, x, 2, z), 0u);
				firstCount += id == 1u;
				changedCount += id != idAt(*other, x, 1, z);
			}
		}
	}
	EXPECT_GT(firstCount, 740u);
	EXPECT_LT(firstCount, 890u);
	EXPECT_GT(changedCount, 100u);

	weighted.flush();
	for (const int cx : {1, 0, -1, -2})
	{
		auto concurrentLower = std::async(std::launch::async, [&] {
			return sameSeed.generate({cx, 0, -1});
		});
		const auto upper = sameSeed.generate({cx, 1, -1});
		const auto lower = concurrentLower.get();
		const auto regenerated = weighted.generate({cx, 1, -1});
		ASSERT_TRUE(upper && lower && regenerated);
		for (std::uint32_t z = 0; z < 16; ++z)
		{
			for (std::uint32_t x = 0; x < 16; ++x)
			{
				const auto id = idAt(*regenerated, x, 1, z);
				EXPECT_EQ(idAt(*upper, x, 1, z), id);
				EXPECT_EQ(idAt(*lower, x, 0, z), id);
			}
		}
	}
}

TEST(VoxelBiomeGeneratorTest, RegistryBindingIsRequiredAndCanBeDetached)
{
	Generator generator;
	generator.bind(nullptr);
	EXPECT_THROW((void)generator.generate({0, 0, 0}), spk::Exception);
	EXPECT_EQ(generator.samples.load(), 0);
	generator.bind(&registry());
	const auto first = generator.generate({0, 0, 0});
	ASSERT_TRUE(first);
	EXPECT_EQ(generator.samples.load(), 289);
	generator.bind(nullptr);
	EXPECT_THROW((void)generator.generate({0, 1, 0}), spk::Exception);
	generator.bind(&registry());
	const auto second = generator.generate({0, 1, 0});
	ASSERT_TRUE(second);
	EXPECT_EQ(generator.samples.load(), 578);
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
	using Data = Generator::Data;
	using Coordinate = spk::World::ChunkCoordinate;

	class SurfaceProbe : public spk::World::SurfacePass<Chunk, Data>
	{
	private:
		std::vector<int> &_trace;

	protected:
		void _applySurface(const spk::Vector2Int &coordinate, Data &data) override
		{
			EXPECT_EQ(coordinate, (spk::Vector2Int{3, -2}));
			const auto &column = std::as_const(data).column(coordinate);
			EXPECT_TRUE(static_cast<bool>(column.influences));
			EXPECT_TRUE(static_cast<bool>(column.heights));
			EXPECT_TRUE(static_cast<bool>(column.strata));
			_trace.push_back(1);
			_trace.push_back(3);
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

		void apply(const Coordinate &coordinate, Data &data) override
		{
			// Appended passes execute after the built-in voxel application pass.
			data.chunk(coordinate).builder.set({0, 0, 0}, 4);
			EXPECT_EQ(coordinate.x, 3);
			EXPECT_EQ(coordinate.z, -2);
			_trace.push_back(2);
		}
	};

	Generator generator;
	std::vector<int> trace;
	generator.addPass<SurfaceProbe>(trace);
	generator.addPass<ChunkProbe>(trace);
	const auto first = generator.generate({3, 0, -2});
	const auto second = generator.generate({3, 1, -2});
	ASSERT_TRUE(first && second);
	EXPECT_EQ(idAt(*first, 0, 0, 0), 4u);
	EXPECT_EQ(idAt(*second, 0, 0, 0), 4u);
	EXPECT_EQ(trace, (std::vector<int>{1, 3, 2, 2}));
	generator.flush();
	ASSERT_TRUE(generator.generate({3, 2, -2}));
	EXPECT_EQ(trace, (std::vector<int>{1, 3, 2, 2, 1, 3, 2}));
}

TEST(VoxelBiomeGeneratorTest, FailedSurfacePassDoesNotPublishPartialColumn)
{
	class FailingPass : public Generator::SurfacePass
	{
	private:
		bool _first = true;

	protected:
		void _applySurface(const spk::Vector2Int &coordinate, Generator::Data &data) override
		{
			auto &column = data.column(coordinate);
			auto &height = column.heights->sample({0, 0});
			EXPECT_FLOAT_EQ(height, 17.75f);
			if (std::exchange(_first, false))
			{
				height += 1.0f;
				throw spk::Exception("Injected surface failure");
			}
		}
	};

	Generator generator;
	generator.insertPass<FailingPass>(2);
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
	spk::World::LocalChunkRequester<Chunk, Generator::Data> requester(workers, generator);
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

		void apply(const spk::World::ChunkCoordinate &, Generator::Data &) override
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

TEST(VoxelStratumInstanceTest, ResolvesAbsoluteHeightsAndVoxelIDs)
{
	using Instance = spk::World::Voxel::Strata::Instance;
	Biome biome;
	auto instance = Instance::obtain(biome, {0, 0}, 17.75);
	EXPECT_EQ(instance->voxelIDAt(18), 0u);
	EXPECT_EQ(instance->voxelIDAt(17), 1u);
	EXPECT_EQ(instance->voxelIDAt(16), 2u);
	EXPECT_EQ(instance->voxelIDAt(15), 2u);
	EXPECT_EQ(instance->voxelIDAt(14), 3u);
	EXPECT_EQ(instance->voxelIDAt(-200), 3u);
	// Resolving a profile does not require the world's registry.
	biome.strata().front().materials = {{5, 1}};
	auto unregistered = Instance::obtain(biome, {0, 0}, 17.75);
	EXPECT_EQ(unregistered->voxelIDAt(17), 5u);
	biome.strata().front().materials = {{1, 1}};
	EXPECT_THROW((void)instance->voxelIDAt(std::numeric_limits<double>::infinity()), spk::Exception);

	biome.strata().resize(2);
	auto finite = Instance::obtain(biome, {0, 0}, -2.25);
	EXPECT_EQ(finite->voxelIDAt(-2), 0u);
	EXPECT_EQ(finite->voxelIDAt(-3), 1u);
	EXPECT_EQ(finite->voxelIDAt(-4), 2u);
	EXPECT_EQ(finite->voxelIDAt(-5), 2u);
	EXPECT_EQ(finite->voxelIDAt(-6), 0u);

	biome.strata().clear();
	auto empty = Instance::obtain(biome, {0, 0}, 0);
	EXPECT_EQ(empty->voxelIDAt(0), 0u);
	EXPECT_EQ(empty->voxelIDAt(-100), 0u);
}

TEST(VoxelStratumInstanceTest, ReusesGridAndInstancesWithoutKeepingOldLayers)
{
	using Instance = spk::World::Voxel::Strata::Instance;
	using Grid = spk::World::Voxel::Strata::Grid;
	Biome biome;
	auto grid = Grid::obtain({2, 1});
	(*grid)[{0, 0}] = Instance::obtain(biome, {0, 0}, 17);
	(*grid)[{1, 0}] = Instance::obtain(biome, {1, 0}, 29);
	const auto *gridAddress = grid.get();
	const auto *first = (*grid)[{0, 0}].get();
	const auto *second = (*grid)[{1, 0}].get();
	EXPECT_NE(first, second);
	grid = {};

	auto recycled = Grid::obtain({2, 1});
	EXPECT_EQ(recycled.get(), gridAddress);
	EXPECT_FALSE(static_cast<bool>((*recycled)[{0, 0}]));
	EXPECT_FALSE(static_cast<bool>((*recycled)[{1, 0}]));
	biome.strata().back().thickness = Biome::Stratum::Thickness::Fixed{2};
	(*recycled)[{0, 0}] = Instance::obtain(biome, {0, 0}, -10);
	(*recycled)[{1, 0}] = Instance::obtain(biome, {1, 0}, 40);
	const auto *a = (*recycled)[{0, 0}].get();
	const auto *b = (*recycled)[{1, 0}].get();
	EXPECT_TRUE((a == first && b == second) || (a == second && b == first));
	EXPECT_EQ(a->voxelIDAt(-10), 1u);
	EXPECT_EQ(a->voxelIDAt(-100), 0u);
	EXPECT_EQ(b->voxelIDAt(40), 1u);
	EXPECT_EQ(b->voxelIDAt(17), 0u);
}

TEST(VoxelStratumInstanceTest, FailedResolutionReturnsInstanceAndClearsPartialLayers)
{
	using Instance = spk::World::Voxel::Strata::Instance;
	Biome biome;
	auto initial = Instance::obtain(biome, {0, 0}, 0);
	const auto *address = initial.get();
	initial = {};
	biome.strata()[1].materials.clear();
	EXPECT_THROW((void)Instance::obtain(biome, {0, 0}, 50), spk::Exception);
	biome.strata()[1].materials = {{2, 1}};
	auto recovered = Instance::obtain(biome, {0, 0}, -1);
	EXPECT_EQ(recovered.get(), address);
	EXPECT_EQ(recovered->voxelIDAt(50), 0u);
	EXPECT_EQ(recovered->voxelIDAt(-1), 1u);
	EXPECT_EQ(recovered->voxelIDAt(-2), 2u);
	EXPECT_EQ(recovered->voxelIDAt(-4), 3u);
}

TEST(VoxelBiomeGeneratorTest, StrataUseFinalHeightMapAfterSurfaceEdits)
{
	class RaiseSurface : public Generator::SurfacePass
	{
	protected:
		void _applySurface(const spk::Vector2Int &coordinate, Generator::Data &data) override
		{
			auto &grid = *data.column(coordinate).heights;
			const auto size = grid.sampleSize();
			for (std::uint32_t z = 0; z < size.y; ++z)
			{
				for (std::uint32_t x = 0; x < size.x; ++x)
				{
					grid.sample({x, z}) += 16.0f;
				}
			}
		}
	};

	class EditedGenerator : public Generator
	{
	public:
		EditedGenerator()
		{
			insertPass<RaiseSurface>(2);
		}
	};

	EditedGenerator generator;
	const auto lower = generator.generate({-1, 1, 0});
	const auto upper = generator.generate({-1, 2, 0});
	ASSERT_TRUE(lower && upper);
	EXPECT_EQ(idAt(*lower, 0, 15, 0), 2u);
	EXPECT_EQ(idAt(*upper, 0, 0, 0), 2u);
	EXPECT_EQ(idAt(*upper, 0, 1, 0), 1u);
	EXPECT_EQ(idAt(*upper, 0, 2, 0), 0u);
}
