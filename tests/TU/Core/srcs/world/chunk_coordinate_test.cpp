#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>

#include "world/chunk.hpp"

namespace
{
	using TestChunk = spk::World::Chunk<32, 16, 8>;
}

TEST(ChunkCoordinateTest, ThreeDimensionsAndExactBoundaries)
{
	static_assert(TestChunk::Size == spk::Vector3Int{32, 16, 8});

	EXPECT_EQ(TestChunk::worldToChunkCoordinate(spk::Vector3Int{0, 0, 0}), (spk::World::ChunkCoordinate{0, 0, 0}));
	EXPECT_EQ(TestChunk::worldToChunkCoordinate(spk::Vector3Int{31, 15, 7}), (spk::World::ChunkCoordinate{0, 0, 0}));
	EXPECT_EQ(TestChunk::worldToChunkCoordinate(spk::Vector3Int{32, 16, 8}), (spk::World::ChunkCoordinate{1, 1, 1}));
	EXPECT_EQ(TestChunk::worldToChunkCoordinate(spk::Vector3Int{-1, -1, -1}), (spk::World::ChunkCoordinate{-1, -1, -1}));
	EXPECT_EQ(TestChunk::worldToChunkCoordinate(spk::Vector3Int{-32, -16, -8}), (spk::World::ChunkCoordinate{-1, -1, -1}));
	EXPECT_EQ(TestChunk::worldToChunkCoordinate(spk::Vector3Int{-33, -17, -9}), (spk::World::ChunkCoordinate{-2, -2, -2}));

	EXPECT_EQ(TestChunk::worldToLocalCoordinate(spk::Vector3Int{-1, -1, -1}), (spk::Vector3Int{31, 15, 7}));
	EXPECT_EQ(TestChunk::worldToLocalCoordinate(spk::Vector3Int{-32, -16, -8}), (spk::Vector3Int{0, 0, 0}));
	EXPECT_EQ(TestChunk::worldToLocalCoordinate(spk::Vector3Int{-33, -17, -9}), (spk::Vector3Int{31, 15, 7}));
}

TEST(ChunkCoordinateTest, ConvertsContinuousWorldCoordinatesUsingFloor)
{
	EXPECT_EQ(TestChunk::worldToChunkCoordinate(spk::Vector3{-0.1f, -16.0f, 7.999f}), (spk::World::ChunkCoordinate{-1, -1, 0}));
	EXPECT_EQ(TestChunk::worldToChunkCoordinate(spk::Vector3{-32.01f, -16.01f, -8.01f}), (spk::World::ChunkCoordinate{-2, -2, -2}));

	const spk::Vector3 local = TestChunk::worldToLocalCoordinate(spk::Vector3{-0.25f, -16.0f, 8.5f});
	EXPECT_FLOAT_EQ(local.x, 31.75f);
	EXPECT_FLOAT_EQ(local.y, 0.0f);
	EXPECT_FLOAT_EQ(local.z, 0.5f);
}

TEST(ChunkCoordinateTest, RoundTripsExactIntegerWorldPositions)
{
	for (const spk::World::ChunkCoordinate coordinate : {
			 spk::World::ChunkCoordinate{-100, 0, 100},
			 spk::World::ChunkCoordinate{0, 0, 0},
			 spk::World::ChunkCoordinate{1, -1, 2},
			 spk::World::ChunkCoordinate{std::numeric_limits<std::int32_t>::min(), 0, std::numeric_limits<std::int32_t>::max()}})
	{
		const spk::World::ChunkWorldCoordinate origin = TestChunk::chunkCoordinateToWorld(coordinate);
		EXPECT_EQ(TestChunk::worldToChunkCoordinate(origin), coordinate);
		EXPECT_EQ(TestChunk::worldToLocalCoordinate(origin), (spk::Vector3Int{0, 0, 0}));
		const spk::World::ChunkWorldCoordinate last{origin.x + 31, origin.y + 15, origin.z + 7};
		EXPECT_EQ(TestChunk::worldToChunkCoordinate(last), coordinate);
	}
}

TEST(ChunkCoordinateTest, HandlesInt64LimitsWithoutOverflow)
{
	const auto min = std::numeric_limits<std::int64_t>::min();
	const auto max = std::numeric_limits<std::int64_t>::max();

	EXPECT_EQ(TestChunk::worldToLocalCoordinate(spk::World::ChunkWorldCoordinate{min, min, min}), (spk::Vector3Int{0, 0, 0}));
	EXPECT_EQ(TestChunk::worldToLocalCoordinate(spk::World::ChunkWorldCoordinate{max, max, max}), (spk::Vector3Int{31, 15, 7}));
	EXPECT_THROW((void)TestChunk::worldToChunkCoordinate(spk::World::ChunkWorldCoordinate{min, 0, 0}), spk::Exception);
	EXPECT_THROW((void)TestChunk::worldToChunkCoordinate(spk::World::ChunkWorldCoordinate{max, 0, 0}), spk::Exception);
}

TEST(ChunkCoordinateTest, HandlesLargeSignedWorldCoordinates)
{
	constexpr auto max = std::numeric_limits<std::int32_t>::max();
	constexpr auto min = std::numeric_limits<std::int32_t>::min();
	EXPECT_EQ(TestChunk::worldToChunkCoordinate(spk::Vector3Int{min, max, min}), (spk::World::ChunkCoordinate{-67108864, 134217727, -268435456}));
	EXPECT_EQ(TestChunk::worldToLocalCoordinate(spk::Vector3Int{min, max, min}), (spk::Vector3Int{0, 15, 0}));
}

TEST(ChunkCoordinateTest, RejectsInvalidFloatingPositionsAndOutOfRangeIndices)
{
	const float infinity = std::numeric_limits<float>::infinity();
	const float nan = std::numeric_limits<float>::quiet_NaN();

	EXPECT_THROW((void)TestChunk::worldToChunkCoordinate(spk::Vector3{infinity, 0, 0}), spk::Exception);
	EXPECT_THROW((void)TestChunk::worldToLocalCoordinate(spk::Vector3{nan, 0, 0}), spk::Exception);
	EXPECT_THROW((void)TestChunk::worldToChunkCoordinate(spk::TVector3<double>{1.0e100, 0, 0}), spk::Exception);
}

TEST(ChunkCoordinateTest, CanBeUsedAsAnOrderedMapKey)
{
	std::map<spk::World::ChunkCoordinate, int> chunks;
	chunks.emplace(spk::World::ChunkCoordinate{1, 0, 0}, 3);
	chunks.emplace(spk::World::ChunkCoordinate{-1, 0, 0}, 1);
	chunks.emplace(spk::World::ChunkCoordinate{0, 0, 0}, 2);

	EXPECT_EQ(chunks.begin()->second, 1);
	EXPECT_EQ(chunks.rbegin()->second, 3);
}
