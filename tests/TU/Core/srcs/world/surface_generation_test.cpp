#include <gtest/gtest.h>

#include <atomic>
#include <future>
#include <limits>

#include "world/height_map.hpp"
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
	EXPECT_EQ(grid->dominant({0, 0}), nullptr);
	grid->at({0, 0}) = {{{&a, 0.5f}, {&b, 0.5f}}};
	EXPECT_EQ(grid->dominant({0, 0}), &a);
	grid->at({0, 0})[0].weight = 0;
	EXPECT_EQ(grid->dominant({0, 0}), &b);
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
