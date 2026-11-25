#include <gtest/gtest.h>
#include "container/grid_2d.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <future>
#include <limits>

#include "world/height_map.hpp"
#include "world/pass/height_map_generation_pass.hpp"
#include "world/pass/surface_biome_distribution_pass.hpp"
#include "world/pass/surface_generation_pass.hpp"
#include "world/pass/surface_height_combination_pass.hpp"
#include "world/surface_biome.hpp"

namespace
{
	using Coordinate = spk::World::ChunkCoordinate;
	using HeightMap = spk::World::HeightMap;

	class ConstantBiome : public spk::World::SurfaceBiome
	{
	private:
		float _height;

	public:
		explicit ConstantBiome(float height) :
			_height(height)
		{
		}
		float groundHeight(const spk::Vector2 &) const override
		{
			return _height;
		}
	};

	struct Chunk
	{
		struct Builder
		{
			int value = 0;
			Chunk build() &&
			{
				return {value};
			}
		};
		int value;
	};

	struct SurfaceData
	{
		using InfluenceGrid = spk::World::SurfaceBiome::Influence::Grid<2>;
		InfluenceGrid::Lease influences;
		HeightMap::Grid::Lease heights;
		int value = 0;
	};
	struct Data
	{
		std::shared_ptr<const SurfaceData> surface;
	};

	using GeneratorBase = spk::World::Generator<Chunk, Data>;
	using SurfaceBase = spk::World::SurfaceGenerationPass<Chunk, Data, SurfaceData>;

	class Surface : public SurfaceBase
	{
	protected:
		Handle &_surfaceDestination(Data &data) override
		{
			return data.surface;
		}
	};

	class Distribution : public spk::World::SurfaceBiomeDistributionPass<SurfaceData, 2>
	{
	private:
		ConstantBiome _first{20};
		ConstantBiome _second{100};

	protected:
		Configuration _gridConfiguration(const spk::Vector2Int &, const SurfaceData &) override
		{
			return {{16, 16}, 1};
		}
		Grid::Lease &_gridDestination(SurfaceData &data) override
		{
			return data.influences;
		}
		void _computeBiomePlacement(const spk::Vector2 &, Cell &cell) override
		{
			cell[0] = {&_first, 3.0f};
			cell[1] = {&_second, 1.0f};
		}
	};

	class Combination : public spk::World::SurfaceHeightCombinationPass<SurfaceData, 2>
	{
	protected:
		const InfluenceGrid &_influences(const SurfaceData &data) override
		{
			return *data.influences;
		}
		Grid::Lease &_gridDestination(SurfaceData &data) override
		{
			return data.heights;
		}
	};

	class CountPass : public spk::World::SurfacePass<SurfaceData>
	{
	private:
		std::atomic<int> &_calls;
		std::atomic<bool> &_fail;

	public:
		CountPass(std::atomic<int> &calls, std::atomic<bool> &fail) :
			_calls(calls),
			_fail(fail)
		{
		}
		void apply(const spk::Vector2Int &, SurfaceData &data) override
		{
			++_calls;
			if (_fail.exchange(false))
			{
				throw spk::Exception("Injected surface failure");
			}
			data.value = static_cast<int>(data.heights->at({0, 0}));
		}
	};

	class Application : public GeneratorBase::Pass
	{
	public:
		void apply(Chunk::Builder &builder, const Coordinate &, Data &data) override
		{
			builder.value = data.surface->value;
		}
	};

	class Generator : public GeneratorBase
	{
	protected:
		std::optional<Chunk::Builder> _createChunkBuilder(const Coordinate &) override
		{
			return Chunk::Builder{};
		}
		Data _createGenerationData() override
		{
			return {};
		}

	public:
		std::atomic<int> calls{0};
		std::atomic<bool> fail{false};

		Generator()
		{
			auto &surface = addPass<Surface>();
			surface.addPass<Distribution>();
			surface.addPass<Combination>();
			surface.addPass<CountPass>(calls, fail);
			addPass<Application>();
		}
	};
}

TEST(SurfaceGridTest, HeightGridEndpointsInterpolationAndPooling)
{
	HeightMap map;
	map.setBaseHeight(7.5f);
	auto grid = map.generate({{16, 16}, 2}, {-16, 0});
	EXPECT_EQ(grid->layout().count(), 81u);
	EXPECT_GE(grid->capacity(), 128u);
	EXPECT_FLOAT_EQ(grid->at({16, 16}), 7.5f);
	grid->at({0, 0}) = 0;
	grid->at({2, 0}) = 10;
	EXPECT_FLOAT_EQ(grid->at({1, 0}, HeightMap::Grid::Interpolation::Nearest), 0);
	EXPECT_FLOAT_EQ(grid->at({1, 0}, HeightMap::Grid::Interpolation::Bilinear), 5);
	EXPECT_FLOAT_EQ(grid->at({16, 16}, HeightMap::Grid::Interpolation::Bilinear), 7.5f);
	EXPECT_THROW((void)grid->at({16.1f, 0}), spk::Exception);
	EXPECT_THROW((void)map.generate({{16, 16}, 3}, {}), spk::Exception);
	EXPECT_THROW((void)map.generate({{16, 16}, 0}, {}), spk::Exception);
	const auto *previous = grid.get();
	grid = {};
	auto recycled = map.generate({{16, 16}, 2}, {});
	EXPECT_EQ(recycled.get(), previous);
	EXPECT_FLOAT_EQ(recycled->at({0, 0}), 7.5f);
}

TEST(SurfaceGridTest, DominantUsesFirstPositiveMaximumAndReflectsEdits)
{
	ConstantBiome a(0), b(1);
	auto grid = SurfaceData::InfluenceGrid::obtain({{16, 16}, 1}, {});
	EXPECT_EQ(grid->dominant({0, 0}, SurfaceData::InfluenceGrid::Interpolation::Bilinear), nullptr);
	grid->at({0, 0}) = {{{&a, 0.5f}, {&b, 0.5f}}};
	EXPECT_EQ(grid->dominant({0, 0}, SurfaceData::InfluenceGrid::Interpolation::Bilinear), &a);
	grid->at({0, 0})[0].weight = 0;
	EXPECT_EQ(grid->dominant({0, 0}, SurfaceData::InfluenceGrid::Interpolation::Bilinear), &b);
}

TEST(SurfaceGenerationTest, ConcurrentVerticalJobsShareOneWeightedHeight)
{
	Generator generator;
	generator.retain({-1, 0, 2});
	generator.retain({-1, 1, 2});
	auto first = std::async(std::launch::async, [&] {
		return generator.generate({-1, 0, 2});
	});
	auto second = std::async(std::launch::async, [&] {
		return generator.generate({-1, 1, 2});
	});
	EXPECT_EQ(first.get()->value, 40);
	EXPECT_EQ(second.get()->value, 40);
	EXPECT_EQ(generator.calls.load(), 1);
	generator.release({-1, 0, 2});
	EXPECT_EQ(generator.generate({-1, 1, 2})->value, 40);
	EXPECT_EQ(generator.calls.load(), 1);
	generator.release({-1, 1, 2});
	EXPECT_EQ(generator.generate({-1, 0, 2})->value, 40);
	EXPECT_EQ(generator.calls.load(), 2);
}

TEST(SurfaceGenerationTest, QueuedReservationSurvivesLastInterestAndFailedBuildRetries)
{
	Generator generator;
	generator.retain({0, 0, 0});
	EXPECT_EQ(generator.generate({0, 0, 0})->value, 40);
	{
		auto reservation = generator.reserve({0, 1, 0});
		generator.release({0, 0, 0});
		EXPECT_EQ(generator.generate({0, 1, 0})->value, 40);
		EXPECT_EQ(generator.calls.load(), 1);
	}
	generator.retain({0, 0, 0});
	generator.fail = true;
	EXPECT_THROW((void)generator.generate({0, 0, 0}), spk::Exception);
	EXPECT_EQ(generator.generate({0, 0, 0})->value, 40);
	EXPECT_EQ(generator.calls.load(), 3);
	generator.release({0, 0, 0});
}

TEST(SurfaceGridTest, AdjacentGridsShareExactAbsoluteBoundarySamples)
{
	HeightMap map;
	map.setBaseHeight(10);
	spk::Perlin a(81);
	a.setFrequency(0.073f);
	spk::Perlin b(913);
	b.setFrequency(0.21f);
	map.noiseLayers().emplace("a", HeightMap::NoiseLayer{a, -4, 8});
	map.noiseLayers().emplace("b", HeightMap::NoiseLayer{b, -2, 2});
	for (float scale : {0.5f, 1.0f, 2.0f, 4.0f})
	{
		auto left = map.generate({{16, 16}, scale}, {-16, -16});
		auto right = map.generate({{16, 16}, scale}, {0, -16});
		auto back = map.generate({{16, 16}, scale}, {-16, 0});
		const auto layout = left->layout();
		for (std::size_t i = 0; i < layout.width; ++i)
		{
			EXPECT_FLOAT_EQ(left->sample(layout.width - 1, i), right->sample(0, i));
			EXPECT_FLOAT_EQ(left->sample(i, layout.height - 1), back->sample(i, 0));
		}
		EXPECT_FLOAT_EQ(left->at({16, 16}), map.sample({0, 0}));
	}
}

TEST(SurfaceGridTest, BilinearUsesBothAxesAndRejectsInvalidCoordinates)
{
	auto grid = HeightMap::Grid::obtain({{4, 2}, 2}, {});
	grid->sample(0, 0) = 0;
	grid->sample(1, 0) = 8;
	grid->sample(0, 1) = 4;
	grid->sample(1, 1) = 12;
	EXPECT_FLOAT_EQ(grid->at({1, 1}, HeightMap::Grid::Interpolation::Bilinear), 6);
	EXPECT_FLOAT_EQ(grid->at({1, 1}, HeightMap::Grid::Interpolation::Nearest), 0);
	EXPECT_FLOAT_EQ(grid->at({1.1f, 1.1f}, HeightMap::Grid::Interpolation::Nearest), 12);
	EXPECT_THROW((void)grid->at({-0.01f, 0}), spk::Exception);
	EXPECT_THROW((void)grid->at({0, std::numeric_limits<float>::infinity()}), spk::Exception);
	EXPECT_THROW((void)grid->at({std::numeric_limits<float>::quiet_NaN(), 0}), spk::Exception);
	EXPECT_THROW((void)HeightMap::Grid::obtain({{16, 16}, std::numeric_limits<float>::infinity()}, {}), spk::Exception);
}

TEST(SurfaceGridTest, RecycledInfluenceGridResetsEverySlot)
{
	ConstantBiome biome(1);
	using Grid = SurfaceData::InfluenceGrid;
	auto grid = Grid::obtain({{8, 8}, 1}, {-8, 0});
	grid->at({0, 0})[0] = {&biome, 1};
	grid->at({8, 8})[1] = {&biome, 1};
	const auto *address = grid.get();
	grid = {};
	auto recycled = Grid::obtain({{8, 8}, 1}, {16, -16});
	EXPECT_EQ(recycled.get(), address);
	EXPECT_EQ(recycled->worldOrigin(), (spk::Vector2{16, -16}));
	for (std::size_t z = 0; z < recycled->layout().height; ++z)
	{
		for (std::size_t x = 0; x < recycled->layout().width; ++x)
		{
			for (const auto &influence : recycled->sample(x, z))
			{
				EXPECT_EQ(influence.biome, nullptr);
				EXPECT_FLOAT_EQ(influence.weight, 0);
			}
		}
	}
}

TEST(SurfaceGenerationTest, HeightCombinationRejectsMissingAndInvalidContributionsWithoutReplacingResult)
{
	SurfaceData data;
	data.influences = SurfaceData::InfluenceGrid::obtain({{2, 2}, 1}, {});
	data.heights = HeightMap::Grid::obtain({{2, 2}, 1}, {});
	data.heights->at({0, 0}) = 99;
	const auto *old = data.heights.get();
	Combination pass;
	EXPECT_THROW(pass.apply({}, data), spk::Exception);
	EXPECT_EQ(data.heights.get(), old);
	ConstantBiome valid(20);
	ConstantBiome invalid(std::numeric_limits<float>::quiet_NaN());
	const auto layout = data.influences->layout();
	for (std::size_t z = 0; z < layout.height; ++z)
	{
		for (std::size_t x = 0; x < layout.width; ++x)
		{
			data.influences->sample(x, z) = {{{&valid, 1}, {&invalid, 0}}};
		}
	}
	EXPECT_NO_THROW(pass.apply({}, data));
	EXPECT_FLOAT_EQ(data.heights->at({0, 0}), 20);
	data.influences->sample(2, 2)[1].weight = 1;
	const auto *previous = data.heights.get();
	EXPECT_THROW(pass.apply({}, data), spk::Exception);
	EXPECT_EQ(data.heights.get(), previous);
}

TEST(SurfaceGenerationTest, DifferentColumnsHaveIndependentCacheLifetimes)
{
	Generator generator;
	generator.retain({-1, 0, 0});
	generator.retain({0, 0, 0});
	auto a = std::async(std::launch::async, [&] {
		return generator.generate({-1, 0, 0});
	});
	auto b = std::async(std::launch::async, [&] {
		return generator.generate({0, 0, 0});
	});
	ASSERT_TRUE(a.get());
	ASSERT_TRUE(b.get());
	EXPECT_EQ(generator.calls.load(), 2);
	generator.release({-1, 0, 0});
	EXPECT_TRUE(generator.generate({0, 2, 0}));
	EXPECT_EQ(generator.calls.load(), 2);
	EXPECT_TRUE(generator.generate({-1, 0, 0}));
	EXPECT_EQ(generator.calls.load(), 3);
	generator.release({0, 0, 0});
}

namespace
{
	class StandaloneHeightPass : public spk::World::HeightMapGenerationPass<Chunk, SurfaceData>
	{
	protected:
		HeightMap::Grid::Configuration _gridConfiguration(const Coordinate &, const SurfaceData &) override
		{
			return {{16, 8}, 2};
		}
		HeightMap::Grid::Lease &_gridDestination(SurfaceData &data) override
		{
			return data.heights;
		}
	};
}

TEST(SurfaceGenerationTest, StandaloneHeightMapPassUsesWorldOriginAndRequiresBinding)
{
	StandaloneHeightPass pass;
	SurfaceData data;
	Chunk::Builder builder;
	EXPECT_THROW(pass.apply(builder, {-2, 7, 3}, data), spk::Exception);
	HeightMap map;
	map.setBaseHeight(12);
	spk::Perlin noise(72);
	noise.setFrequency(0.07f);
	map.noiseLayers().emplace("relief", HeightMap::NoiseLayer{noise, -3, 3});
	pass.bind(map);
	pass.apply(builder, {-2, 7, 3}, data);
	ASSERT_TRUE(data.heights);
	EXPECT_EQ(data.heights->worldOrigin(), (spk::Vector2{-32, 24}));
	EXPECT_FLOAT_EQ(data.heights->at({16, 8}), map.sample({-16, 32}));
}

TEST(SurfaceGridTest, BilinearInfluenceCombinesByBiomeIdentityRatherThanSlot)
{
	ConstantBiome a(0), b(1);
	using Grid = SurfaceData::InfluenceGrid;
	auto grid = Grid::obtain({{4, 4}, 4}, {});
	grid->sample(0, 0) = {{{&a, 1}, {&b, 0}}};
	grid->sample(1, 0) = {{{&b, 1}, {&a, 0}}};
	grid->sample(0, 1) = {{{&b, 0}, {&a, 1}}};
	grid->sample(1, 1) = {{{&a, 0}, {&b, 1}}};
	const auto cell = grid->interpolate({3, 1});
	EXPECT_EQ(cell[0].biome, &a);
	EXPECT_FLOAT_EQ(cell[0].weight, 0.25f);
	EXPECT_EQ(cell[1].biome, &b);
	EXPECT_FLOAT_EQ(cell[1].weight, 0.75f);
	EXPECT_EQ(grid->dominant({3, 1}, SurfaceData::InfluenceGrid::Interpolation::Bilinear), &b);
	EXPECT_EQ(grid->dominant({3, 1}, Grid::Interpolation::Floor), &a);
	EXPECT_EQ(grid->dominant({2, 2}, SurfaceData::InfluenceGrid::Interpolation::Bilinear), &a);
	EXPECT_EQ(grid->dominant({4, 4}, SurfaceData::InfluenceGrid::Interpolation::Bilinear), &b);
}

TEST(SurfaceGridTest, InterpolatedInfluencesPreserveAllFourCornerBiomes)
{
	using Grid = spk::World::SurfaceBiome::Influence::Grid<1>;
	ConstantBiome a(1), b(2), c(3), d(4);
	auto grid = Grid::obtain({{4, 4}, 4}, {});
	grid->sample(0, 0)[0] = {&a, 1};
	grid->sample(1, 0)[0] = {&b, 1};
	grid->sample(0, 1)[0] = {&c, 1};
	grid->sample(1, 1)[0] = {&d, 1};
	const auto center = grid->interpolate({2, 2});
	EXPECT_EQ(center[0].biome, &a);
	EXPECT_EQ(center[1].biome, &b);
	EXPECT_EQ(center[2].biome, &c);
	EXPECT_EQ(center[3].biome, &d);
	for (const auto &influence : center)
	{
		EXPECT_FLOAT_EQ(influence.weight, 0.25f);
	}
	EXPECT_EQ(grid->dominant({3, 3}, Grid::Interpolation::Bilinear), &d);
}

TEST(SurfaceGridTest, InterpolationSharesChunkEdgesAndKeepsExactNodeTieOrder)
{
	using Grid = SurfaceData::InfluenceGrid;
	ConstantBiome a(1), b(2);
	auto left = Grid::obtain({{4, 4}, 4}, {-4, 0});
	auto right = Grid::obtain({{4, 4}, 4}, {0, 0});
	left->sample(1, 0) = right->sample(0, 0) = {{{&b, 0.5f}, {&a, 0.5f}}};
	left->sample(1, 1) = right->sample(0, 1) = {{{&a, 1}, {&b, 0}}};
	EXPECT_EQ(left->dominant({4, 0}, SurfaceData::InfluenceGrid::Interpolation::Bilinear), &b);
	EXPECT_EQ(right->dominant({0, 0}, SurfaceData::InfluenceGrid::Interpolation::Bilinear), &b);
	for (float z : {0.0f, 1.0f, 2.0f, 3.0f, 4.0f})
	{
		const auto l = left->interpolate({4, z});
		const auto r = right->interpolate({0, z});
		for (std::size_t i = 0; i < l.size(); ++i)
		{
			EXPECT_EQ(l[i].biome, r[i].biome);
			EXPECT_FLOAT_EQ(l[i].weight, r[i].weight);
		}
		EXPECT_EQ(left->dominant({4, z}, SurfaceData::InfluenceGrid::Interpolation::Bilinear), right->dominant({0, z}, SurfaceData::InfluenceGrid::Interpolation::Bilinear));
	}
}

TEST(Grid2DTest, ReusesCapacityAcrossShapesAndResetsCells)
{
	using Grid = spk::Grid2D<int>;
	auto grid = Grid::obtain(3, 5);
	const auto *address = grid.get();
	EXPECT_GE(grid->capacity(), 16u);
	EXPECT_EQ(grid->layout().count(), 15u);
	grid->at(2, 4) = 42;
	grid = {};
	auto recycled = Grid::obtain(4, 4);
	EXPECT_EQ(recycled.get(), address);
	EXPECT_EQ(recycled->layout().width, 4u);
	EXPECT_EQ(recycled->layout().height, 4u);
	for (std::size_t y = 0; y < 4; ++y)
	{
		for (std::size_t x = 0; x < 4; ++x)
		{
			EXPECT_EQ(recycled->at(x, y), 0);
		}
	}
	EXPECT_THROW((void)recycled->at(4, 0), spk::Exception);
	EXPECT_THROW((void)recycled->at(0, 4), spk::Exception);
	EXPECT_THROW((void)Grid::obtain(0, 4), spk::Exception);
	EXPECT_THROW((void)Grid::obtain(std::numeric_limits<std::size_t>::max(), 2), spk::Exception);
}

TEST(Grid2DTest, ReleasesElementsAndKeepsConcurrentLeasesIndependent)
{
	using Grid = spk::Grid2D<std::shared_ptr<int>>;
	auto first = Grid::obtain(2, 2);
	auto second = Grid::obtain(2, 2);
	EXPECT_NE(first.get(), second.get());
	first->at(0, 0) = std::make_shared<int>(7);
	std::weak_ptr<int> observer = first->at(0, 0);
	EXPECT_FALSE(second->at(0, 0));
	first = {};
	EXPECT_TRUE(observer.expired());
	const auto &readOnly = second;
	EXPECT_FALSE(readOnly->at(0, 0));
}
