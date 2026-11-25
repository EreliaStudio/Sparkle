#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/context/update_context.hpp"
#include "engine/engine_headless.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "threading/worker_pool.hpp"
#include "voxel/chunk.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_filter.hpp"
#include "world/chunk_interest.hpp"
#include "world/generator.hpp"
#include "world/local_chunk_requester.hpp"
#include "world/world_observer.hpp"
#include "world/world_streaming_system.hpp"

namespace
{
	using namespace std::chrono_literals;

	using Chunk = spk::Voxel::Chunk<8, 16, 32>;
	using Coordinate = spk::World::ChunkCoordinate;
	using Volume = spk::Voxel::Volume;
	using Streaming = spk::World::WorldStreamingSystem<Chunk>;
	using Registry = spk::Voxel::Definition::Registry;

	[[nodiscard]] Registry makeDefinitions()
	{
		spk::Voxel::Shape::Builder shapeBuilder;
		shapeBuilder.addSlot("surface");
		const spk::Voxel::Shape shape = shapeBuilder.build();

		Registry::Builder builder;
		builder.add(1, spk::Voxel::Definition(shape, {{"surface", {"ground"}}}));
		builder.add(2, spk::Voxel::Definition(shape, {{"surface", {"wall"}}}));
		return builder.build();
	}

	struct GenerationData
	{
		Registry definitions;
		std::int64_t seed;
		std::mutex mutex;
		std::map<Coordinate, Volume::Builder> builders;

		Volume::Builder &builder(const Coordinate &coordinate)
		{
			const std::scoped_lock lock(mutex);
			return builders.at(coordinate);
		}

		GenerationData(const Registry &registry, std::int64_t seedValue) :
			definitions(registry),
			seed(seedValue)
		{
		}
	};

	using Generator = spk::World::Generator<Chunk, GenerationData>;

	class TerrainPass final : public Generator::Pass
	{
	public:
		void apply(const Coordinate &coordinate, GenerationData &data) override
		{
			auto &volume = data.builder(coordinate);
			volume.bind(data.definitions);
			const auto origin = Chunk::chunkCoordinateToWorld(coordinate);
			for (std::uint32_t z = 0; z < static_cast<std::uint32_t>(Chunk::Size.z); ++z)
			{
				for (std::uint32_t x = 0; x < static_cast<std::uint32_t>(Chunk::Size.x); ++x)
				{
					const std::int64_t value = (origin.x + x) * 13 + (origin.z + z) * 7 + data.seed;
					const std::int64_t height = (value % 3 + 3) % 3;
					for (std::uint32_t y = 0; y < static_cast<std::uint32_t>(Chunk::Size.y); ++y)
					{
						if (origin.y + y <= height)
						{
							volume.set({x, y, z}, 1);
						}
					}
				}
			}
		}
	};

	class WallPass final : public Generator::Pass
	{
	public:
		void apply(const Coordinate &coordinate, GenerationData &data) override
		{
			auto &volume = data.builder(coordinate);
			for (std::int64_t x = -2; x <= 10; ++x)
			{
				for (std::int64_t y = 3; y <= 5; ++y)
				{
					const spk::World::ChunkWorldCoordinate world{x, y, 5};
					if (Chunk::worldToChunkCoordinate(world) != coordinate)
					{
						continue;
					}
					volume.set(spk::Vector3UInt(Chunk::worldToLocalCoordinate(world)), 2);
				}
			}
		}
	};

	class WorldGenerator final : public Generator
	{
	private:
		[[nodiscard]] bool _prepareGenerationData(const Coordinate &coordinate, GenerationData &data) override
		{
			if (coordinate.x < -1 || coordinate.x > 1 || coordinate.y != 0 || coordinate.z != 0)
			{
				return false;
			}
			const std::scoped_lock lock(data.mutex);
			if (!data.builders.try_emplace(coordinate, spk::Vector3UInt{Chunk::Size}).second)
			{
				throw spk::Exception("Chunk generation is already active");
			}
			return true;
		}

		[[nodiscard]] Chunk _instantiateChunk(const Coordinate &coordinate, GenerationData &data) override
		{
			Chunk chunk;
			chunk.setVolume(data.builder(coordinate).build());
			return chunk;
		}

		void _cleanGenerationData(const Coordinate &coordinate, GenerationData &data) noexcept override
		{
			const std::scoped_lock lock(data.mutex);
			data.builders.erase(coordinate);
		}

	public:
		explicit WorldGenerator(std::int64_t seed) :
			Generator(makeDefinitions(), seed)
		{
			addPass<TerrainPass>();
			addPass<WallPass>();
		}
	};

	template <typename T>
	concept HasChunkBuilder = requires { typename T::Builder; };

	static_assert(!HasChunkBuilder<Chunk>);
	static_assert(std::is_copy_constructible_v<Chunk>);
	static_assert(std::is_move_constructible_v<Chunk>);
	static_assert(std::is_copy_assignable_v<Chunk>);
	static_assert(std::is_move_assignable_v<Chunk>);
	static_assert(std::is_same_v<decltype(std::declval<Chunk &>().volume()), const Volume &>);

	[[nodiscard]] Chunk makeChunk(std::uint32_t id = 1)
	{
		Volume::Builder builder({8, 16, 32});
		builder.bind(makeDefinitions());
		builder.set({0, 0, 0}, id);
		Chunk chunk;
		chunk.setVolume(builder.build());
		return chunk;
	}

	[[nodiscard]] Volume changedVolume(const Chunk &chunk)
	{
		Volume::Builder builder(chunk.volume().dimensions());
		builder.load(chunk.volume());
		builder.set({1, 2, 3}, 2);
		return builder.build();
	}

	[[nodiscard]] std::vector<std::uint32_t> snapshot(const Chunk &chunk)
	{
		std::vector<std::uint32_t> result;
		for (std::uint32_t z = 0; z < 32; ++z)
		{
			for (std::uint32_t y = 0; y < 16; ++y)
			{
				for (std::uint32_t x = 0; x < 8; ++x)
				{
					result.push_back(chunk.volume().atRaw({x, y, z}));
				}
			}
		}
		return result;
	}

	[[nodiscard]] const Chunk &chunkAtWorld(const spk::World::ChunkCollection<Chunk> &collection, const spk::World::ChunkWorldCoordinate &world)
	{
		return collection.at(Chunk::worldToChunkCoordinate(world));
	}

	[[nodiscard]] bool isSolid(const spk::World::ChunkCollection<Chunk> &collection, const spk::World::ChunkWorldCoordinate &world)
	{
		const auto local = Chunk::worldToLocalCoordinate(world);
		return !chunkAtWorld(collection, world).volume().at(spk::Vector3UInt(local)).isAir();
	}

	class StreamingFixture
	{
	public:
		spk::EngineHeadless engine;
		WorldGenerator generator{17};
		spk::WorkerPool workers{2};
		spk::World::LocalChunkRequester<Chunk, GenerationData> requester{workers, generator};
		spk::World::ChunkCollection<Chunk> collection;
		spk::Entity3D player{"Voxel observer"};

		StreamingFixture()
		{
			collection.attach(engine);
			player.transform().place({0.0f, 0.0f, 5.0f});
			player.addComponent<spk::World::WorldObserver>(spk::World::ChunkInterest({1, 0, 0}));
			engine.addEntity(&player);
			engine.addSystem<Streaming>(collection, requester);
		}

		~StreamingFixture()
		{
			if (Streaming *streaming = engine.getSystem<Streaming>(); streaming != nullptr)
			{
				engine.removeSystem(*streaming);
			}
			engine.removeEntity(&player);
			collection.detach();
		}

		void tick(int millisecond)
		{
			spk::UpdateContext context{.time = std::chrono::milliseconds(millisecond), .deltaTime = 1ms};
			engine.updateState(context);
		}

		[[nodiscard]] bool load()
		{
			for (int i = 0; i < 500 && collection.size() < 3; ++i)
			{
				tick(i);
				std::this_thread::sleep_for(1ms);
			}
			return collection.size() == 3;
		}
	};
}

TEST(VoxelChunkTest, NonCubicDimensionsAreExactlyThoseOfTheVolume)
{
	using NonCubic = spk::Voxel::Chunk<8, 16, 32>;
	Volume::Builder builder({8, 16, 32});
	NonCubic chunk;
	EXPECT_EQ(chunk.volume().atRaw({0, 0, 0}), Volume::PackedCell::Air);
	chunk.setVolume(builder.build());

	EXPECT_EQ(NonCubic::Size, (spk::Vector3Int{8, 16, 32}));
	EXPECT_EQ(chunk.volume().dimensions(), (spk::Vector3UInt{8, 16, 32}));
	EXPECT_TRUE(chunk.volume().at({7, 15, 31}).isAir());
	EXPECT_THROW((void)chunk.volume().at({8, 0, 0}), spk::Exception);
	EXPECT_THROW(chunk.setVolume(Volume::Builder({8, 16, 31}).build()), spk::Exception);
}

TEST(VoxelChunkTest, SetVolumeAlwaysSignalsReplacement)
{
	Chunk chunk = makeChunk();
	const Volume original = chunk.volume();
	int editions = 0;
	auto subscription = chunk.subscribeToChunkEdition([&]() {
		++editions;
	});

	chunk.setVolume(original);
	EXPECT_EQ(editions, 1);
	chunk.setVolume(changedVolume(chunk));
	EXPECT_EQ(editions, 2);
	EXPECT_EQ(chunk.volume().atRaw({1, 2, 3}), Volume::PackedCell::encode(2, spk::Voxel::Cell::HorizontalOrientation::XPositive, spk::Voxel::Cell::VerticalOrientation::YPositive));
	EXPECT_EQ(original.atRaw({1, 2, 3}), Volume::PackedCell::Air);
	chunk.setVolume(chunk.volume());
	EXPECT_EQ(editions, 3);
	EXPECT_THROW(chunk.setVolume(Volume::Builder({2, 2, 2}).build()), spk::Exception);
	EXPECT_EQ(editions, 3);
	EXPECT_TRUE(subscription.isValid());
}

TEST(VoxelChunkTest, VolumeBuilderLoadsExistingSnapshotAndIsConsumedByBuild)
{
	Chunk source = makeChunk();
	const auto first = source.volume().atRaw({0, 0, 0});
	Volume::Builder builder(source.volume().dimensions());
	builder.load(source.volume());
	builder.erase({0, 0, 0});
	builder.set({7, 15, 31}, 2);
	Chunk changed;
	changed.setVolume(builder.build());

	EXPECT_EQ(source.volume().atRaw({0, 0, 0}), first);
	EXPECT_EQ(changed.volume().atRaw({0, 0, 0}), Volume::PackedCell::Air);
	EXPECT_FALSE(changed.volume().at({7, 15, 31}).isAir());
	EXPECT_THROW((void)builder.build(), spk::Exception);
	EXPECT_THROW(builder.load(source.volume()), spk::Exception);
}

TEST(VoxelChunkTest, CopiesMovesAndAssignmentsKeepIndependentEditionSubscriptions)
{
	Chunk original = makeChunk();
	int originalEditions = 0;
	auto originalSubscription = original.subscribeToChunkEdition([&]() {
		++originalEditions;
	});
	Chunk copy(original);
	int copyEditions = 0;
	auto copySubscription = copy.subscribeToChunkEdition([&]() {
		++copyEditions;
	});
	Chunk moved(std::move(copy));
	int movedEditions = 0;
	auto movedSubscription = moved.subscribeToChunkEdition([&]() {
		++movedEditions;
	});

	moved.setVolume(changedVolume(moved));
	EXPECT_EQ(originalEditions, 0);
	EXPECT_EQ(copyEditions, 0);
	EXPECT_EQ(movedEditions, 1);

	Chunk assigned = makeChunk();
	int assignedEditions = 0;
	auto assignedSubscription = assigned.subscribeToChunkEdition([&]() {
		++assignedEditions;
	});
	assigned = original;
	assigned.setVolume(changedVolume(assigned));
	EXPECT_EQ(assignedEditions, 1);
	EXPECT_EQ(originalEditions, 0);
	original.setVolume(changedVolume(original));
	EXPECT_EQ(originalEditions, 1);
	EXPECT_EQ(movedEditions, 1);
	EXPECT_TRUE(originalSubscription.isValid());
	EXPECT_TRUE(movedSubscription.isValid());
	EXPECT_TRUE(assignedSubscription.isValid());
}

TEST(VoxelChunkTest, CollectionRelaysChunkEditionAndPreservesFilterAcrossReplacement)
{
	spk::EngineHeadless engine;
	spk::World::ChunkCollection<Chunk> collection;
	collection.attach(engine);
	const Coordinate coordinate{-1, 0, 0};
	int editions = 0;
	auto contract = collection.subscribeToChunkEdition([&](const Coordinate &changed) {
		EXPECT_EQ(changed, coordinate);
		++editions;
	});

	collection.addChunk(coordinate, makeChunk());
	ASSERT_EQ(editions, 1);
	auto &entry = collection.begin()->second;
	auto *filter = entry.entity().getComponent<spk::World::ChunkFilter<Chunk>>();
	ASSERT_NE(filter, nullptr);
	EXPECT_EQ(&filter->chunk(), &collection.at(coordinate));

	collection.at(coordinate).setVolume(changedVolume(collection.at(coordinate)));
	EXPECT_EQ(editions, 2);
	collection.addChunk(coordinate, makeChunk());
	EXPECT_EQ(editions, 3);
	EXPECT_EQ(entry.entity().getComponent<spk::World::ChunkFilter<Chunk>>(), filter);
	EXPECT_EQ(&filter->chunk(), &collection.at(coordinate));
	collection.at(coordinate).setVolume(changedVolume(collection.at(coordinate)));
	EXPECT_EQ(editions, 4);
	EXPECT_TRUE(contract.isValid());
	collection.detach();
}

TEST(VoxelChunkDemoTest, MissingChunksReturnNullopt)
{
	WorldGenerator generator(17);
	EXPECT_FALSE(generator.generate({2, 0, 0}).has_value());
	EXPECT_FALSE(generator.generate({-2, 0, 0}).has_value());
	EXPECT_FALSE(generator.generate({0, -1, 0}).has_value());
	EXPECT_FALSE(generator.generate({0, 0, 1}).has_value());
}

TEST(VoxelChunkDemoTest, GeneratingInReverseOrderProducesIdenticalVolumes)
{
	WorldGenerator generator(17);
	const std::vector<Coordinate> positions{{-1, 0, 0}, {0, 0, 0}, {1, 0, 0}};
	std::map<Coordinate, std::vector<std::uint32_t>> expected;
	for (const Coordinate &coordinate : positions)
	{
		const std::optional<Chunk> chunk = generator.generate(coordinate);
		ASSERT_TRUE(chunk.has_value());
		expected.emplace(coordinate, snapshot(*chunk));
	}

	for (auto it = positions.rbegin(); it != positions.rend(); ++it)
	{
		const std::optional<Chunk> chunk = generator.generate(*it);
		ASSERT_TRUE(chunk.has_value());
		EXPECT_EQ(snapshot(*chunk), expected.at(*it));
	}
}

TEST(VoxelChunkDemoTest, GeneratedChunksMatchTerrainAndWallAtEveryCell)
{
	WorldGenerator generator(17);
	for (std::int32_t chunkX = -1; chunkX <= 1; ++chunkX)
	{
		const Coordinate coordinate{chunkX, 0, 0};
		const std::optional<Chunk> chunk = generator.generate(coordinate);
		ASSERT_TRUE(chunk.has_value());

		for (std::uint32_t z = 0; z < 32; ++z)
		{
			for (std::uint32_t y = 0; y < 16; ++y)
			{
				for (std::uint32_t x = 0; x < 8; ++x)
				{
					const std::int64_t worldX = static_cast<std::int64_t>(chunkX) * 8 + x;
					const std::int64_t value = worldX * 13 + static_cast<std::int64_t>(z) * 7 + 17;
					const std::int64_t height = (value % 3 + 3) % 3;
					const bool wall = worldX >= -2 && worldX <= 10 && y >= 3 && y <= 5 && z == 5;
					const std::uint32_t expected = wall ? 2 : (y <= height ? 1 : 0);
					const std::uint32_t actual = Volume::PackedCell::definitionId(chunk->volume().atRaw({x, y, z}));
					EXPECT_EQ(actual, expected) << "world=(" << worldX << ", " << y << ", " << z << ")";
				}
			}
		}
	}
}

TEST(VoxelChunkDemoTest, NegativeWorldCoordinatesAndStructureCrossChunkBoundaries)
{
	WorldGenerator generator(17);
	spk::World::ChunkCollection<Chunk> collection;
	for (const Coordinate &coordinate : std::vector<Coordinate>{{1, 0, 0}, {-1, 0, 0}, {0, 0, 0}})
	{
		std::optional<Chunk> chunk = generator.generate(coordinate);
		ASSERT_TRUE(chunk.has_value());
		collection.addChunk(coordinate, std::move(*chunk));
	}

	EXPECT_EQ(Chunk::worldToChunkCoordinate(spk::World::ChunkWorldCoordinate{-1, 4, 5}), (Coordinate{-1, 0, 0}));
	EXPECT_EQ(Chunk::worldToLocalCoordinate(spk::World::ChunkWorldCoordinate{-1, 4, 5}), (spk::Vector3Int{7, 4, 5}));
	for (std::int64_t x = -8; x <= 15; ++x)
	{
		EXPECT_TRUE(isSolid(collection, {x, 0, 5}));
		EXPECT_EQ(isSolid(collection, {x, 4, 5}), x >= -2 && x <= 10) << "world X = " << x;
	}
	EXPECT_TRUE(isSolid(collection, {-1, 3, 5}));
	EXPECT_TRUE(isSolid(collection, {0, 3, 5}));
	EXPECT_TRUE(isSolid(collection, {7, 5, 5}));
	EXPECT_TRUE(isSolid(collection, {8, 5, 5}));
	EXPECT_TRUE(isSolid(collection, {10, 5, 5}));
	EXPECT_FALSE(isSolid(collection, {11, 5, 5}));
}

TEST(VoxelChunkDemoTest, HeadlessEngineStreamsGeneratedChunksToLiveChunkFilters)
{
	StreamingFixture fixture;
	int editions = 0;
	const std::thread::id ownerThread = std::this_thread::get_id();
	auto subscription = fixture.collection.subscribeToChunkEdition([&](const Coordinate &) {
		EXPECT_EQ(std::this_thread::get_id(), ownerThread);
		++editions;
	});

	ASSERT_TRUE(fixture.load());
	EXPECT_EQ(editions, 3);
	for (const auto &[coordinate, entry] : fixture.collection)
	{
		const auto *filter = entry.entity().getComponent<spk::World::ChunkFilter<Chunk>>();
		ASSERT_NE(filter, nullptr);
		const Chunk *chunk = nullptr;
		ASSERT_TRUE(filter->tryGet(chunk));
		EXPECT_EQ(chunk, &entry.chunk());
		EXPECT_EQ(chunk->volume().dimensions(), (spk::Vector3UInt{8, 16, 32}));
		EXPECT_EQ(coordinate.y, 0);
	}
	EXPECT_TRUE(isSolid(fixture.collection, {-2, 4, 5}));
	EXPECT_TRUE(isSolid(fixture.collection, {0, 4, 5}));
	EXPECT_TRUE(isSolid(fixture.collection, {8, 4, 5}));
	fixture.collection.at({0, 0, 0}).setVolume(changedVolume(fixture.collection.at({0, 0, 0})));
	EXPECT_EQ(editions, 4);
	EXPECT_TRUE(subscription.isValid());
}
