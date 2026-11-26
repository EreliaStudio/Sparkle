#include "container/grid_2d.hpp"
#include <utility>
#include <cstdint>
#include <gtest/gtest.h>
#include <type_traits>
#include <vector>

#include <array>
#include <atomic>
#include <cmath>
#include <future>
#include <limits>

#include "world/height_map.hpp"
#include "world/pass/height_map_generation_pass.hpp"
#include "world/pass/surface_biome_distribution_pass.hpp"
#include "world/pass/surface_height_combination_pass.hpp"
#include "world/surface_biome.hpp"
#include "world/voxel/biome_generation_data.hpp"

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
		int value;
	};

	using GenerationData = spk::World::Voxel::Biome::GenerationData<2>;
	using SurfaceData = GenerationData::Column;

	struct StandaloneData : SurfaceData
	{
		void flush()
		{
		}
	};

	class Distribution : public spk::World::SurfaceBiomeDistributionPass<Chunk, GenerationData, 2>
	{
	private:
		ConstantBiome _first{20};
		ConstantBiome _second{100};
		spk::World::SurfaceBiomeDistributionPass<Chunk, GenerationData, 2>::Distribution _influenceFunction = [this](const spk::Vector2 &, Cell &cell) {
			cell[0] = {&_first, 3.0f};
			cell[1] = {&_second, 1.0f};
		};

	public:
		Distribution()
		{
			bind(&_influenceFunction);
		}

	protected:
		Configuration _computeGridConfiguration(const spk::Vector2Int &, const GenerationData &) override
		{
			return {{16, 16}, 1};
		}
		void _storeInfluenceGrid(const spk::Vector2Int &coordinate, GenerationData &data, Grid::Lease grid) override
		{
			data.column(coordinate).influences = std::move(grid);
		}
	};

	using CombinationBase = spk::World::SurfaceHeightCombinationPass<Chunk, StandaloneData, 2>;
	class Combination : public CombinationBase
	{
	public:
		using CombinationBase::CombinationBase;

	protected:
		const InfluenceGrid &_influenceGrid(const spk::Vector2Int &, const StandaloneData &data) override
		{
			return *data.influences;
		}
		void _storeHeightMap(const spk::Vector2Int &, StandaloneData &data, Grid::Lease grid) override
		{
			data.heights = std::move(grid);
		}
	};

	using ColumnCombinationBase = spk::World::SurfaceHeightCombinationPass<Chunk, GenerationData, 2>;
	class ColumnCombination : public ColumnCombinationBase
	{
	public:
		using ColumnCombinationBase::ColumnCombinationBase;

	protected:
		const InfluenceGrid &_influenceGrid(const spk::Vector2Int &coordinate, const GenerationData &data) override
		{
			return *data.column(coordinate).influences;
		}

		void _storeHeightMap(const spk::Vector2Int &coordinate, GenerationData &data, Grid::Lease grid) override
		{
			data.column(coordinate).heights = std::move(grid);
		}
	};
}

namespace
{
	class InterpolationProbe : public spk::World::SurfaceGrid<int>
	{
	public:
		using Lease = spk::Pool<InterpolationProbe>::Lease;
		using SurfaceGrid::_forEachInterpolationSample;

		static Lease obtain(const Configuration &configuration)
		{
			return _obtain<InterpolationProbe>(configuration, {}, []() {
				return new InterpolationProbe();
			},
											   [](InterpolationProbe &) {
											   });
		}
	};
}

TEST(SurfaceGridTest, SharedInterpolationVisitsOnlyContributingSamples)
{
	auto grid = InterpolationProbe::obtain({{4, 4}, 2});
	grid->sample({0, 0}) = 1;
	grid->sample({1, 0}) = 2;
	grid->sample({0, 1}) = 3;
	grid->sample({1, 1}) = 4;
	std::vector<std::pair<int, double>> visits;
	const auto visit = [&](const int &value, double weight) {
		visits.emplace_back(value, weight);
	};
	using Visits = decltype(visits);
	grid->_forEachInterpolationSample({0, 0}, visit);
	EXPECT_EQ(visits, (Visits{{1, 1.0}}));
	visits.clear();
	grid->_forEachInterpolationSample({1, 0}, visit);
	EXPECT_EQ(visits, (Visits{{1, 0.5}, {2, 0.5}}));
	visits.clear();
	grid->_forEachInterpolationSample({0, 1}, visit);
	EXPECT_EQ(visits, (Visits{{1, 0.5}, {3, 0.5}}));
	visits.clear();
	grid->_forEachInterpolationSample({1, 1}, visit);
	EXPECT_EQ(visits, (Visits{{1, 0.25}, {2, 0.25}, {3, 0.25}, {4, 0.25}}));
	visits.clear();
	EXPECT_EQ(grid->at({1.5f, 1.5f}), 1);
	EXPECT_TRUE(visits.empty());
	EXPECT_THROW(grid->_forEachInterpolationSample({-1, 0}, visit), spk::Exception);
	EXPECT_TRUE(visits.empty());
	auto decimal = InterpolationProbe::obtain({{1, 1}, 0.1f});
	decimal->sample({7, 7}) = 7;
	decimal->_forEachInterpolationSample(decimal->localPosition({7, 7}), visit);
	EXPECT_EQ(visits, (Visits{{7, 1.0}}));
}

TEST(SurfaceGridTest, HeightGridAlignedInterpolationDoesNotReadUnusedValues)
{
	using Grid = HeightMap::Grid;
	auto grid = Grid::obtain({{2, 2}, 2}, {});
	grid->sample({0, 0}) = 2;
	grid->sample({1, 0}) = 6;
	grid->sample({0, 1}) = std::numeric_limits<float>::quiet_NaN();
	grid->sample({1, 1}) = std::numeric_limits<float>::quiet_NaN();
	EXPECT_FLOAT_EQ(grid->interpolate({0, 0}), 2);
	EXPECT_FLOAT_EQ(grid->interpolate({1, 0}), 4);
	EXPECT_FLOAT_EQ(grid->interpolate({2, 0}), 6);
}

TEST(SurfaceGridTest, HeightGridEndpointsInterpolationAndPooling)
{
	HeightMap map;
	map.setBaseHeight(7.5f);
	auto grid = map.generate({{16, 16}, 2}, {-16, 0});
	EXPECT_EQ(grid->count(), 81u);
	EXPECT_EQ(grid->sampleSize(), (spk::Vector2UInt{9, 9}));
	EXPECT_GE(grid->capacity(), 128u);
	EXPECT_FLOAT_EQ(grid->at({16, 16}), 7.5f);
	grid->at({0, 0}) = 0;
	grid->at({2, 0}) = 10;
	EXPECT_FLOAT_EQ(grid->at({1, 0}), 0);
	EXPECT_FLOAT_EQ(grid->interpolate({1, 0}), 5);
	EXPECT_FLOAT_EQ(grid->interpolate({16, 16}), 7.5f);
	EXPECT_THROW((void)grid->at({16.1f, 0}), spk::Exception);
	EXPECT_THROW((void)map.generate({{16, 16}, 3}, {}), spk::Exception);
	EXPECT_THROW((void)map.generate({{16, 16}, 0}, {}), spk::Exception);
	const auto *previous = grid.get();
	grid = {};
	auto recycled = map.generate({{16, 16}, 2}, {});
	EXPECT_EQ(recycled.get(), previous);
	EXPECT_FLOAT_EQ(recycled->at({0, 0}), 7.5f);
}

TEST(SurfaceGridTest, InfluenceCellDominantWorksWithoutGrid)
{
	using Cell = spk::World::SurfaceBiome::Influence::Cell<4>;
	ConstantBiome a(1), b(2);
	Cell cell{};
	EXPECT_EQ(cell.dominant(), nullptr);
	cell[0] = {&a, 0};
	cell[1] = {nullptr, 1};
	EXPECT_EQ(cell.dominant(), nullptr);
	cell[0].weight = 0.5f;
	cell[2] = {&b, 0.5f};
	EXPECT_EQ(std::as_const(cell).dominant(), &a);
	cell[2].weight = 0.75f;
	EXPECT_EQ(cell.dominant(), &b);
	EXPECT_EQ(cell.size(), 4u);
}

TEST(SurfaceGridTest, DominantUsesFirstPositiveMaximumAndReflectsEdits)
{
	ConstantBiome a(0), b(1);
	auto grid = SurfaceData::InfluenceGrid::obtain({{16, 16}, 1}, {});
	EXPECT_EQ(grid->interpolate({0, 0}).dominant(), nullptr);
	grid->at({0, 0}) = {{{{&a, 0.5f}, {&b, 0.5f}}}};
	EXPECT_EQ(grid->interpolate({0, 0}).dominant(), &a);
	grid->at({0, 0})[0].weight = 0;
	EXPECT_EQ(grid->interpolate({0, 0}).dominant(), &b);
}

TEST(SurfaceGenerationTest, ConcurrentRequestsShareOneWeightedColumnUntilFlush)
{
	const spk::Voxel::Definition::Registry registry;
	GenerationData data;
	data.bind(&registry);
	const Coordinate lower{-1, 0, 2};
	const Coordinate upper{-1, 1, 2};
	Distribution distribution;
	ColumnCombination combination(ColumnCombinationBase::Combination::Weighted);
	std::atomic<int> calls{0};
	const auto generate = [&](const Coordinate &coordinate) {
		data.prepareChunk(coordinate, {16, 16, 16});
		if (!std::as_const(data).column(coordinate).ready())
		{
			++calls;
			distribution.apply(coordinate, data);
			combination.apply(coordinate, data);
			data.completeChunk(coordinate);
		}
		const float height = std::as_const(data).column(coordinate).heights->at({0, 0});
		data.cleanChunk(coordinate);
		return height;
	};
	auto first = std::async(std::launch::async, [&] {
		return generate(lower);
	});
	auto second = std::async(std::launch::async, [&] {
		return generate(upper);
	});
	EXPECT_FLOAT_EQ(first.get(), 40);
	EXPECT_FLOAT_EQ(second.get(), 40);
	EXPECT_EQ(calls.load(), 1);
	EXPECT_FLOAT_EQ(generate(lower), 40);
	EXPECT_EQ(calls.load(), 1);
	data.flush();
	EXPECT_EQ(data.columnCount(), 0u);
	EXPECT_FLOAT_EQ(generate(lower), 40);
	EXPECT_EQ(calls.load(), 2);
}

TEST(SurfaceGenerationTest, FailedColumnBuildCanBeRetried)
{
	const spk::Voxel::Definition::Registry registry;
	GenerationData data;
	data.bind(&registry);
	const Coordinate coordinate{0, 0, 0};
	Distribution distribution;
	ColumnCombination combination(ColumnCombinationBase::Combination::Weighted);
	data.prepareChunk(coordinate, {16, 16, 16});
	EXPECT_FALSE(std::as_const(data).column(coordinate).heights);
	distribution.apply(coordinate, data);
	EXPECT_FALSE(std::as_const(data).column(coordinate).ready());
	// A failed job cleans its partially constructed column without publishing it.
	data.cleanChunk(coordinate);
	data.prepareChunk(coordinate, {16, 16, 16});
	EXPECT_FALSE(std::as_const(data).column(coordinate).heights);
	EXPECT_FALSE(static_cast<bool>(std::as_const(data).column(spk::Vector2Int{0, 0}).influences));
	EXPECT_FALSE(std::as_const(data).column(coordinate).ready());
	distribution.apply(coordinate, data);
	combination.apply(coordinate, data);
	data.completeChunk(coordinate);
	EXPECT_FLOAT_EQ(std::as_const(data).column(coordinate).heights->at({0, 0}), 40);
	data.cleanChunk(coordinate);
	data.prepareChunk(coordinate, {16, 16, 16});
	EXPECT_TRUE(std::as_const(data).column(coordinate).heights);
	EXPECT_FLOAT_EQ(std::as_const(data).column(coordinate).heights->at({0, 0}), 40);
	data.cleanChunk(coordinate);
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
		const auto layout = left->sampleSize();
		for (std::uint32_t i = 0; i < layout.x; ++i)
		{
			EXPECT_FLOAT_EQ(left->sample({layout.x - 1, i}), right->sample({0, i}));
			EXPECT_FLOAT_EQ(left->sample({i, layout.y - 1}), back->sample({i, 0}));
		}
		EXPECT_FLOAT_EQ(left->at({16, 16}), map.sample({0, 0}));
	}
}

TEST(SurfaceGridTest, BilinearUsesBothAxesAndRejectsInvalidCoordinates)
{
	auto grid = HeightMap::Grid::obtain({{4, 2}, 2}, {});
	grid->sample({0, 0}) = 0;
	grid->sample({1, 0}) = 8;
	grid->sample({0, 1}) = 4;
	grid->sample({1, 1}) = 12;
	EXPECT_FLOAT_EQ(grid->interpolate({1, 1}), 6);
	EXPECT_FLOAT_EQ(grid->at({1.1f, 1.1f}), 0);
	EXPECT_FLOAT_EQ(grid->at({2.9f, 1.9f}), 8);
	EXPECT_FLOAT_EQ(grid->at({2, 2}), 12);
	EXPECT_FLOAT_EQ(grid->at({4, 2}), grid->sample({2, 1}));
	EXPECT_THROW((void)grid->interpolate({-0.01f, 0}), spk::Exception);
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
	for (std::uint32_t z = 0; z < recycled->sampleSize().y; ++z)
	{
		for (std::uint32_t x = 0; x < recycled->sampleSize().x; ++x)
		{
			for (const auto &influence : recycled->sample({x, z}))
			{
				EXPECT_EQ(influence.biome, nullptr);
				EXPECT_FLOAT_EQ(influence.weight, 0);
			}
		}
	}
}

TEST(SurfaceGenerationTest, HeightCombinationRejectsMissingAndInvalidContributionsWithoutReplacingResult)
{
	StandaloneData data;
	data.influences = SurfaceData::InfluenceGrid::obtain({{2, 2}, 1}, {});
	data.heights = HeightMap::Grid::obtain({{2, 2}, 1}, {});
	data.heights->at({0, 0}) = 99;
	const auto *old = data.heights.get();
	Combination pass(CombinationBase::Combination::Weighted);
	EXPECT_THROW(pass.apply({}, data), spk::Exception);
	EXPECT_EQ(data.heights.get(), old);
	ConstantBiome valid(20);
	ConstantBiome invalid(std::numeric_limits<float>::quiet_NaN());
	const auto layout = data.influences->sampleSize();
	for (std::uint32_t z = 0; z < layout.y; ++z)
	{
		for (std::uint32_t x = 0; x < layout.x; ++x)
		{
			data.influences->sample({x, z}) = {{{{&valid, 1}, {&invalid, 0}}}};
		}
	}
	EXPECT_NO_THROW(pass.apply({}, data));
	EXPECT_FLOAT_EQ(data.heights->at({0, 0}), 20);
	data.influences->sample({2, 2})[1].weight = 1;
	const auto *previous = data.heights.get();
	EXPECT_THROW(pass.apply({}, data), spk::Exception);
	EXPECT_EQ(data.heights.get(), previous);
}

TEST(SurfaceGenerationTest, FlushReclaimsOnlyIdleColumns)
{
	const spk::Voxel::Definition::Registry registry;
	GenerationData data;
	data.bind(&registry);
	const Coordinate active{-1, 0, 0};
	const Coordinate idle{0, 0, 0};
	data.prepareChunk(idle, {16, 16, 16});
	EXPECT_FALSE(std::as_const(data).column(idle).ready());
	data.completeChunk(idle);
	data.cleanChunk(idle);

	std::promise<void> entered;
	std::promise<void> resume;
	auto ready = entered.get_future();
	auto gate = resume.get_future().share();
	auto building = std::async(std::launch::async, [&] {
		data.prepareChunk(active, {16, 16, 16});
		entered.set_value();
		gate.wait();
		EXPECT_FALSE(std::as_const(data).column(active).ready());
		data.completeChunk(active);
		data.cleanChunk(active);
	});
	ready.wait();
	data.flush();
	EXPECT_EQ(data.columnCount(), 1u);
	resume.set_value();
	building.get();
	data.prepareChunk(active, {16, 16, 16});
	EXPECT_TRUE(std::as_const(data).column(active).ready());
	data.flush();
	EXPECT_EQ(data.columnCount(), 1u);
	data.cleanChunk(active);
	data.flush();
	EXPECT_EQ(data.columnCount(), 0u);
}

namespace
{
	class StandaloneHeightPass : public spk::World::HeightMapGenerationPass<Chunk, StandaloneData>
	{
	protected:
		HeightMap::Grid::Configuration _gridConfiguration(const Coordinate &, const StandaloneData &) override
		{
			return {{16, 8}, 2};
		}
		HeightMap::Grid::Lease &_gridDestination(const Coordinate &, StandaloneData &data) override
		{
			return data.heights;
		}
	};
}

TEST(SurfaceGenerationTest, StandaloneHeightMapPassUsesWorldOriginAndRequiresBinding)
{
	StandaloneHeightPass pass;
	StandaloneData data;
	EXPECT_THROW(pass.apply({-2, 7, 3}, data), spk::Exception);
	HeightMap map;
	map.setBaseHeight(12);
	spk::Perlin noise(72);
	noise.setFrequency(0.07f);
	map.noiseLayers().emplace("relief", HeightMap::NoiseLayer{noise, -3, 3});
	pass.bind(map);
	pass.apply({-2, 7, 3}, data);
	ASSERT_TRUE(data.heights);
	EXPECT_EQ(data.heights->worldOrigin(), (spk::Vector2{-32, 24}));
	EXPECT_FLOAT_EQ(data.heights->at({16, 8}), map.sample({-16, 32}));
}

TEST(SurfaceGridTest, BilinearInfluenceCombinesByBiomeIdentityRatherThanSlot)
{
	ConstantBiome a(0), b(1);
	using Grid = SurfaceData::InfluenceGrid;
	auto grid = Grid::obtain({{4, 4}, 4}, {});
	grid->sample({0, 0}) = {{{{&a, 1}, {&b, 0}}}};
	grid->sample({1, 0}) = {{{{&b, 1}, {&a, 0}}}};
	grid->sample({0, 1}) = {{{{&b, 0}, {&a, 1}}}};
	grid->sample({1, 1}) = {{{{&a, 0}, {&b, 1}}}};
	const auto cell = grid->interpolate({3, 1});
	EXPECT_EQ(cell[0].biome, &a);
	EXPECT_FLOAT_EQ(cell[0].weight, 0.25f);
	EXPECT_EQ(cell[1].biome, &b);
	EXPECT_FLOAT_EQ(cell[1].weight, 0.75f);
	EXPECT_EQ(grid->interpolate({3, 1}).dominant(), &b);
	EXPECT_EQ(grid->at({3, 1}).dominant(), &a);
	EXPECT_EQ(grid->interpolate({2, 2}).dominant(), &a);
	EXPECT_EQ(grid->interpolate({4, 4}).dominant(), &b);
}

TEST(SurfaceGridTest, InterpolatedInfluencesPreserveAllFourCornerBiomes)
{
	using Grid = spk::World::SurfaceBiome::Influence::Grid<1>;
	ConstantBiome a(1), b(2), c(3), d(4);
	auto grid = Grid::obtain({{4, 4}, 4}, {});
	grid->sample({0, 0})[0] = {&a, 1};
	grid->sample({1, 0})[0] = {&b, 1};
	grid->sample({0, 1})[0] = {&c, 1};
	grid->sample({1, 1})[0] = {&d, 1};
	const auto center = grid->interpolate({2, 2});
	EXPECT_EQ(center[0].biome, &a);
	EXPECT_EQ(center[1].biome, &b);
	EXPECT_EQ(center[2].biome, &c);
	EXPECT_EQ(center[3].biome, &d);
	for (const auto &influence : center)
	{
		EXPECT_FLOAT_EQ(influence.weight, 0.25f);
	}
	EXPECT_EQ(grid->interpolate({3, 3}).dominant(), &d);
}

TEST(SurfaceGridTest, InterpolationSharesChunkEdgesAndKeepsExactNodeTieOrder)
{
	using Grid = SurfaceData::InfluenceGrid;
	ConstantBiome a(1), b(2);
	auto left = Grid::obtain({{4, 4}, 4}, {-4, 0});
	auto right = Grid::obtain({{4, 4}, 4}, {0, 0});
	left->sample({1, 0}) = right->sample({0, 0}) = {{{{&b, 0.5f}, {&a, 0.5f}}}};
	left->sample({1, 1}) = right->sample({0, 1}) = {{{{&a, 1}, {&b, 0}}}};
	EXPECT_EQ(left->interpolate({4, 0}).dominant(), &b);
	EXPECT_EQ(right->interpolate({0, 0}).dominant(), &b);
	for (float z : {0.0f, 1.0f, 2.0f, 3.0f, 4.0f})
	{
		const auto l = left->interpolate({4, z});
		const auto r = right->interpolate({0, z});
		for (std::size_t i = 0; i < l.size(); ++i)
		{
			EXPECT_EQ(l[i].biome, r[i].biome);
			EXPECT_FLOAT_EQ(l[i].weight, r[i].weight);
		}
		EXPECT_EQ(left->interpolate({4, z}).dominant(), right->interpolate({0, z}).dominant());
	}
}

TEST(Grid2DTest, ReusesCapacityAcrossShapesAndResetsCells)
{
	using Grid = spk::Grid2D<int>;
	auto grid = Grid::obtain({3, 5});
	const auto *address = grid.get();
	EXPECT_GE(grid->capacity(), 16u);
	EXPECT_EQ(grid->count(), 15u);
	grid->at({2, 4}) = 42;
	grid = {};
	auto recycled = Grid::obtain({4, 4});
	EXPECT_EQ(recycled.get(), address);
	EXPECT_EQ(recycled->size(), (spk::Vector2UInt{4, 4}));
	EXPECT_EQ(recycled->count(), 16u);
	for (std::size_t y = 0; y < 4; ++y)
	{
		for (std::size_t x = 0; x < 4; ++x)
		{
			EXPECT_EQ(recycled->at({static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y)}), 0);
		}
	}
	EXPECT_THROW((void)recycled->at({4, 0}), spk::Exception);
	EXPECT_THROW((void)recycled->at({0, 4}), spk::Exception);
	EXPECT_THROW((void)Grid::obtain({0, 4}), spk::Exception);
	EXPECT_THROW((void)Grid::obtain({std::numeric_limits<std::uint32_t>::max(), std::numeric_limits<std::uint32_t>::max()}), spk::Exception);
}

TEST(Grid2DTest, ReleasesElementsAndKeepsConcurrentLeasesIndependent)
{
	using Grid = spk::Grid2D<std::shared_ptr<int>>;
	auto first = Grid::obtain({2, 2});
	auto second = Grid::obtain({2, 2});
	EXPECT_NE(first.get(), second.get());
	first->at({0, 0}) = std::make_shared<int>(7);
	std::weak_ptr<int> observer = first->at({0, 0});
	EXPECT_FALSE(second->at({0, 0}));
	first = {};
	EXPECT_TRUE(observer.expired());
	const auto &readOnly = second;
	EXPECT_FALSE(readOnly->at({0, 0}));
}

TEST(Grid2DTest, BracketAccessAndStructuredBindings)
{
	auto grid = spk::Grid2D<int>::obtain({3, 2});
	(*grid)[{1, 0}] = 42;
	EXPECT_EQ(grid->at({1, 0}), 42);
	EXPECT_THROW((void)(*grid)[(spk::Vector2UInt{3, 0})], spk::Exception);

	std::size_t index = 0;
	for (auto &&[coords, value] : *grid)
	{
		static_assert(std::is_const_v<std::remove_reference_t<decltype(coords)>>);
		static_assert(std::is_same_v<decltype(value), int &>);
		EXPECT_EQ(coords.x, index % 3);
		EXPECT_EQ(coords.y, index / 3);
		value = static_cast<int>(++index);
	}
	EXPECT_EQ(index, 6u);

	const auto &readOnly = *grid.get();
	index = 0;
	for (auto &&[coords, value] : readOnly)
	{
		static_assert(std::is_same_v<decltype(value), const int &>);
		EXPECT_EQ(value, static_cast<int>(++index));
		EXPECT_EQ(&value, &readOnly[coords]);
	}
	EXPECT_EQ(index, 6u);
	EXPECT_EQ(readOnly.cbegin(), readOnly.begin());
	EXPECT_EQ(readOnly.cend(), readOnly.end());
}

TEST(Grid2DTest, StructuredBindingsSupportBoolAndMoveOnlyElements)
{
	auto flags = spk::Grid2D<bool>::obtain({2, 2});
	static_assert(std::is_same_v<decltype(flags->at({0, 0})), bool &>);
	static_assert(std::is_same_v<decltype(std::as_const(*flags).at({0, 0})), const bool &>);
	for (auto &&[coords, value] : *flags)
	{
		static_assert(std::is_same_v<decltype(value), bool &>);
		value = coords.x == coords.y;
	}
	EXPECT_TRUE(flags->at({0, 0}));
	EXPECT_FALSE(flags->at({0, 1}));

	auto objects = spk::Grid2D<std::unique_ptr<int>>::obtain({2, 1});
	for (auto &&[coords, value] : *objects)
	{
		value = std::make_unique<int>(static_cast<int>(coords.x));
	}
	EXPECT_EQ(*objects->at({1, 0}), 1);
}

TEST(SurfaceGridTest, SampleReturnsReferencesAndChecksCoordinates)
{
	auto grid = spk::World::HeightMap::Grid::obtain({{2, 2}, 1.0f}, {0, 0});
	static_assert(std::is_same_v<decltype(grid->sample({0, 0})), float &>);
	const auto &readOnly = *grid.get();
	static_assert(std::is_same_v<decltype(readOnly.sample({0, 0})), const float &>);
	grid->sample({1, 1}) = 42.0f;
	EXPECT_EQ(&grid->sample({1, 1}), &grid->at({1, 1}));
	EXPECT_FLOAT_EQ(readOnly.sample({1, 1}), 42.0f);
	EXPECT_THROW((void)grid->sample({3, 0}), spk::Exception);
}

TEST(SurfaceGridTest, LocalPositionUsesSampleCoordinates)
{
	auto grid = spk::World::HeightMap::Grid::obtain({{4, 6}, 0.5f}, {10, 20});
	const auto position = grid->localPosition({4, 6});
	EXPECT_FLOAT_EQ(position.x, 2.0f);
	EXPECT_FLOAT_EQ(position.y, 3.0f);
	const auto endpoint = grid->localPosition({8, 12});
	EXPECT_FLOAT_EQ(endpoint.x, 4.0f);
	EXPECT_FLOAT_EQ(endpoint.y, 6.0f);
	EXPECT_THROW((void)grid->localPosition({9, 0}), spk::Exception);
	EXPECT_THROW((void)grid->localPosition({0, 13}), spk::Exception);
}

TEST(SurfaceGridTest, BilinearAlignedPositionsPreserveInfluences)
{
	using Grid = SurfaceData::InfluenceGrid;
	ConstantBiome a(20), b(100), c(200);
	auto grid = Grid::obtain({{8, 8}, 4}, {});
	grid->sample({1, 1}) = {{{{&a, 0.25f}, {&b, 0.75f}}}};
	grid->sample({2, 1})[0] = {&c, 1};
	grid->sample({1, 2})[0] = {&c, 1};
	grid->sample({2, 2})[0] = {&c, 1};
	const auto exact = grid->interpolate({4, 4});
	EXPECT_EQ(exact[0].biome, &a);
	EXPECT_FLOAT_EQ(exact[0].weight, 0.25f);
	EXPECT_EQ(exact[1].biome, &b);
	EXPECT_FLOAT_EQ(exact[1].weight, 0.75f);
	EXPECT_EQ(exact[2].biome, nullptr);
	for (const auto &position : {spk::Vector2{6, 4}, spk::Vector2{4, 6}})
	{
		const auto edge = grid->interpolate(position);
		EXPECT_EQ(edge[0].biome, &a);
		EXPECT_FLOAT_EQ(edge[0].weight, 0.125f);
		EXPECT_EQ(edge[1].biome, &b);
		EXPECT_FLOAT_EQ(edge[1].weight, 0.375f);
		EXPECT_EQ(edge[2].biome, &c);
		EXPECT_FLOAT_EQ(edge[2].weight, 0.5f);
	}
	grid->sample({1, 1}) = {{{{&a, 0.25f}, {&a, 0.75f}}}};
	const auto merged = grid->interpolate({4, 4});
	EXPECT_EQ(merged[0].biome, &a);
	EXPECT_FLOAT_EQ(merged[0].weight, 1);
	EXPECT_EQ(merged[1].biome, nullptr);
	auto decimal = Grid::obtain({{1, 1}, 0.1f}, {});
	decimal->sample({3, 3})[0] = {&a, 1};
	decimal->sample({4, 3})[0] = {&b, 1};
	const auto decimalExact = decimal->interpolate(decimal->localPosition({3, 3}));
	EXPECT_EQ(decimalExact[0].biome, &a);
	EXPECT_FLOAT_EQ(decimalExact[0].weight, 1);
	EXPECT_EQ(decimalExact[1].biome, nullptr);
	EXPECT_THROW((void)grid->interpolate({9, 4}), spk::Exception);
}

TEST(SurfaceGenerationTest, DominantHeightCombinationUsesWeightRatherThanHeight)
{
	using Mode = CombinationBase::Combination;
	ConstantBiome low(20), high(100), invalid(std::numeric_limits<float>::quiet_NaN());
	StandaloneData data;
	data.influences = SurfaceData::InfluenceGrid::obtain({{1, 1}, 1}, {});
	for (std::uint32_t z = 0; z < 2; ++z)
	{
		for (std::uint32_t x = 0; x < 2; ++x)
		{
			data.influences->sample({x, z}) = {{{{&low, 0.75f}, {&high, 0.25f}}}};
		}
	}
	Combination weighted(Mode::Weighted);
	Combination dominant(Mode::Dominant);
	weighted.apply({}, data);
	EXPECT_FLOAT_EQ(data.heights->sample({0, 0}), 40);
	dominant.apply({}, data);
	EXPECT_FLOAT_EQ(data.heights->sample({0, 0}), 20);
	data.influences->sample({0, 0}) = {{{{&low, 0.5f}, {&invalid, 0.5f}}}};
	EXPECT_NO_THROW(dominant.apply({}, data));
	EXPECT_FLOAT_EQ(data.heights->sample({0, 0}), 20);
	const auto *previous = data.heights.get();
	data.influences->sample({0, 0})[1].weight = 0.75f;
	EXPECT_THROW(dominant.apply({}, data), spk::Exception);
	EXPECT_EQ(data.heights.get(), previous);
	data.influences->sample({0, 0}) = {};
	EXPECT_THROW(dominant.apply({}, data), spk::Exception);
	EXPECT_EQ(data.heights.get(), previous);
}

TEST(SurfaceGenerationTest, MutableColumnAccessRequiresUnpublishedActiveGeneration)
{
	const spk::Voxel::Definition::Registry registry;
	GenerationData data;
	data.bind(&registry);
	const Coordinate first{0, 0, 0};
	const Coordinate second{0, 1, 0};
	const spk::Vector2Int surface{0, 0};
	EXPECT_THROW((void)data.column(first), spk::Exception);

	data.prepareChunk(first, {16, 16, 16});
	EXPECT_EQ(&data.column(first), &data.column(surface));
	EXPECT_FALSE(std::as_const(data).column(first).ready());
	data.completeChunk(first);
	EXPECT_THROW((void)data.column(first), spk::Exception);
	EXPECT_THROW((void)data.column(surface), spk::Exception);
	EXPECT_TRUE(std::as_const(data).column(first).ready());
	data.cleanChunk(first);

	data.prepareChunk(second, {16, 16, 16});
	EXPECT_THROW((void)data.column(second), spk::Exception);
	EXPECT_TRUE(std::as_const(data).column(second).ready());
	EXPECT_EQ(&std::as_const(data).column(second), &std::as_const(data).column(surface));
	data.cleanChunk(second);
	EXPECT_THROW((void)data.column(second), spk::Exception);
	EXPECT_THROW((void)std::as_const(data).column(second), spk::Exception);
}
