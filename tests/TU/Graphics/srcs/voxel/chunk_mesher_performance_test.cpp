#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "voxel/chunk_mesher.hpp"
#include "voxel/occlusion/cache.hpp"
#include "world/chunk_collection.hpp"

namespace
{
	using Coordinate = spk::World::ChunkCoordinate;
	using Registry = spk::Voxel::Definition::Registry;
	using Volume = spk::Voxel::Volume;
	using Clock = std::chrono::steady_clock;

	constexpr int WarmupRuns = 2;
	constexpr std::size_t TimedRuns = 7;

	enum class Pattern
	{
		Solid,
		HollowRooms,
		TerracedCaves,
		BridgesAndTowers,
		Checkerboard
	};

	struct VolumeModel
	{
		spk::Vector3UInt dimensions;
		std::vector<std::uint8_t> cells;

		explicit VolumeModel(spk::Vector3UInt dimensions) :
			dimensions(dimensions),
			cells(static_cast<std::size_t>(dimensions.x) * dimensions.y * dimensions.z, 0)
		{
		}

		[[nodiscard]] std::size_t index(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept
		{
			return x + static_cast<std::size_t>(dimensions.x) * (y + static_cast<std::size_t>(dimensions.y) * z);
		}

		void set(std::uint32_t x, std::uint32_t y, std::uint32_t z, std::uint8_t id)
		{
			cells[index(x, y, z)] = id;
		}

		[[nodiscard]] std::uint8_t at(std::uint32_t x, std::uint32_t y, std::uint32_t z) const noexcept
		{
			return cells[index(x, y, z)];
		}

		[[nodiscard]] bool filled(std::int32_t x, std::int32_t y, std::int32_t z) const noexcept
		{
			if (x < 0 || y < 0 || z < 0 ||
				x >= static_cast<std::int32_t>(dimensions.x) ||
				y >= static_cast<std::int32_t>(dimensions.y) ||
				z >= static_cast<std::int32_t>(dimensions.z))
			{
				return false;
			}
			return at(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), static_cast<std::uint32_t>(z)) != 0;
		}

		[[nodiscard]] std::size_t voxelCount() const noexcept
		{
			return cells.size();
		}
	};

	[[nodiscard]] VolumeModel makeModel(spk::Vector3UInt dimensions, Pattern pattern)
	{
		VolumeModel result(dimensions);
		const auto width = dimensions.x;
		const auto height = dimensions.y;
		const auto depth = dimensions.z;

		for (std::uint32_t z = 0; z < depth; ++z)
		{
			for (std::uint32_t y = 0; y < height; ++y)
			{
				for (std::uint32_t x = 0; x < width; ++x)
				{
					bool filled = false;
					switch (pattern)
					{
					case Pattern::Solid:
						filled = true;
						break;
					case Pattern::HollowRooms:
					{
						const bool shell = x == 0 || x + 1 == width || z == 0 || z + 1 == depth ||
							y == 0 || y + 1 == height;
						const bool partition = (x == width / 2 || z == depth / 2) &&
							y > 0 && y < height / 2;
						const bool doorway = y <= 3 && (
							(z == 0 && x >= width / 2 - 1 && x <= width / 2 + 1) ||
							(x == width / 2 && z >= depth / 3 && z <= depth / 3 + 2) ||
							(z == depth / 2 && x >= width / 3 && x <= width / 3 + 2));
						const bool skylight = y + 1 == height &&
							x > width / 3 && x < 2 * width / 3 &&
							z > depth / 3 && z < 2 * depth / 3;
						filled = (shell || partition) && !doorway && !skylight;
						break;
					}
					case Pattern::TerracedCaves:
					{
						const auto level = height / 4 + (x / 4 + z / 5) % (height / 2 + 1);
						const bool cavern = x > 2 && x + 3 < width && z > 2 && z + 3 < depth &&
							y > 1 && y < level && (x * 3 + z * 5 + y * 7) % 19 < 4;
						filled = y <= level && !cavern;
						break;
					}
					case Pattern::BridgesAndTowers:
					{
						const auto bridgeLevel = height / 2;
						const bool plaza = y == 0;
						const bool tower = x % 7 == 2 && z % 7 == 2 && y <= bridgeLevel + (x + z) % 4;
						const bool span = y == bridgeLevel && (x % 7 == 2 || z % 7 == 2);
						const bool parapet = y == bridgeLevel + 1 && x % 7 == 2 && z % 3 != 0;
						filled = plaza || tower || span || parapet;
						break;
					}
					case Pattern::Checkerboard:
						filled = (x + y + z) % 2 == 0;
						break;
					}
					if (filled)
					{
						result.set(x, y, z, (x + 3 * z) % 7 == 0 ? 2 : 1);
					}
				}
			}
		}
		return result;
	}

	[[nodiscard]] spk::Voxel::Shape loadShape(const char *name)
	{
		spk::Voxel::Shape::Builder builder;
		builder.addFromJSON(spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (std::string(name) + ".json")));
		return builder.build();
	}

	[[nodiscard]] spk::Voxel::Definition makeDefinition(const spk::Voxel::Shape &shape, const char *material)
	{
		spk::Voxel::Definition::SlotMapping slots;
		for (const auto &slot : shape.slots())
		{
			slots.emplace(slot.name, spk::Voxel::Definition::Slot{material, 1.0f});
		}
		return {shape, std::move(slots)};
	}

	[[nodiscard]] Registry makeRegistry()
	{
		Registry::Builder builder;
		const auto cube = loadShape("cube");
		builder.add(1, makeDefinition(cube, "stone"));
		builder.add(2, makeDefinition(cube, "brick"));
		builder.add(3, makeDefinition(loadShape("slab"), "stone"));
		builder.add(4, makeDefinition(loadShape("slope"), "stone"));
		builder.add(5, makeDefinition(loadShape("stair"), "brick"));
		return builder.build();
	}

	[[nodiscard]] Volume makeVolume(const VolumeModel &model, const Registry &registry)
	{
		Volume::Builder builder(model.dimensions);
		builder.bind(registry);
		for (std::uint32_t z = 0; z < model.dimensions.z; ++z)
		{
			for (std::uint32_t y = 0; y < model.dimensions.y; ++y)
			{
				for (std::uint32_t x = 0; x < model.dimensions.x; ++x)
				{
					if (const auto id = model.at(x, y, z); id != 0)
					{
						builder.set({x, y, z}, id);
					}
				}
			}
		}
		return builder.build();
	}

	struct MeshCounts
	{
		std::size_t vertices = 0;
		std::size_t triangles = 0;
		std::size_t opaqueVertices = 0;
		std::size_t transparentVertices = 0;
	};

	[[nodiscard]] MeshCounts countMesh(const spk::Voxel::Geometry &geometry)
	{
		MeshCounts result;
		result.opaqueVertices = geometry.opaqueMesh().vertexCount();
		result.transparentVertices = geometry.transparentMesh().vertexCount();
		result.vertices = result.opaqueVertices + result.transparentVertices;
		result.triangles = (geometry.opaqueMesh().indexCount() + geometry.transparentMesh().indexCount()) / 3;
		return result;
	}

	struct FaceOracle
	{
		std::size_t occupied = 0;
		std::size_t visibleFaces = 0;

		[[nodiscard]] std::size_t vertices() const noexcept
		{
			return visibleFaces * 6;
		}

		[[nodiscard]] std::size_t triangles() const noexcept
		{
			return visibleFaces * 2;
		}
	};

	// Independent of the voxel mesher: each exposed cube face emits two triangles,
	// with three vertices per triangle. Material names do not change opaque occlusion.
	[[nodiscard]] FaceOracle countExposedFaces(const VolumeModel &model)
	{
		constexpr std::array<std::array<int, 3>, 6> Directions{{
			{{-1, 0, 0}},
			{{1, 0, 0}},
			{{0, -1, 0}},
			{{0, 1, 0}},
			{{0, 0, -1}},
			{{0, 0, 1}}
		}};
		FaceOracle result;
		for (std::uint32_t z = 0; z < model.dimensions.z; ++z)
		{
			for (std::uint32_t y = 0; y < model.dimensions.y; ++y)
			{
				for (std::uint32_t x = 0; x < model.dimensions.x; ++x)
				{
					if (model.at(x, y, z) == 0)
					{
						continue;
					}
					++result.occupied;
					for (const auto &direction : Directions)
					{
						if (!model.filled(
								static_cast<std::int32_t>(x) + direction[0],
								static_cast<std::int32_t>(y) + direction[1],
								static_cast<std::int32_t>(z) + direction[2]))
						{
							++result.visibleFaces;
						}
					}
				}
			}
		}
		return result;
	}

	void expectCounts(const MeshCounts &actual, std::size_t expectedVertices, std::size_t expectedTriangles)
	{
		EXPECT_EQ(actual.vertices, expectedVertices);
		EXPECT_EQ(actual.opaqueVertices, expectedVertices);
		EXPECT_EQ(actual.transparentVertices, 0u);
		EXPECT_EQ(actual.triangles, expectedTriangles);
	}

	template <typename Bake>
	void measure(
		std::string_view scene,
		std::size_t chunks,
		std::size_t voxels,
		std::size_t occupied,
		std::size_t expectedVertices,
		std::size_t expectedTriangles,
		Bake &&bake)
	{
		for (int iteration = 0; iteration < WarmupRuns; ++iteration)
		{
			expectCounts(bake(), expectedVertices, expectedTriangles);
		}

		std::array<std::chrono::nanoseconds, TimedRuns> samples{};
		for (std::size_t iteration = 0; iteration < samples.size(); ++iteration)
		{
			const auto start = Clock::now();
			const MeshCounts actual = bake();
			samples[iteration] = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start);
			expectCounts(actual, expectedVertices, expectedTriangles);
		}

		std::sort(samples.begin(), samples.end());
		const double medianNs = static_cast<double>(samples[samples.size() / 2].count());
		const double medianMsPerChunk = medianNs / (1000000.0 * chunks);
		const double medianNsPerVoxel = medianNs / voxels;
		std::cout << std::fixed << std::setprecision(3)
				  << "\n[Chunk meshing] " << scene
				  << " | chunks=" << chunks
				  << " | voxels=" << voxels
				  << " | occupied=" << occupied
				  << " | vertices=" << expectedVertices
				  << " | triangles=" << expectedTriangles
				  << " | median=" << medianMsPerChunk << " ms/chunk"
				  << " | " << medianNsPerVoxel << " ns/voxel"
				  << " | range=" << static_cast<double>(samples.front().count()) / 1000000.0
				  << ".." << static_cast<double>(samples.back().count()) / 1000000.0 << " ms/batch\n";
	}

	template <std::int32_t SX, std::int32_t SY, std::int32_t SZ>
	void measureSingle(std::string_view name, Pattern pattern)
	{
		using Chunk = spk::Voxel::Chunk<SX, SY, SZ>;
		const auto model = makeModel({SX, SY, SZ}, pattern);
		const auto expected = countExposedFaces(model);
		const auto registry = makeRegistry();
		Chunk chunk;
		chunk.setVolume(makeVolume(model, registry));
		spk::World::ChunkCollection<Chunk> collection;
		collection.addChunk({-1, 0, 1}, std::move(chunk));
		spk::Voxel::ChunkMesher<SX, SY, SZ> mesher(collection, {-1, 0, 1});
		spk::Voxel::Occlusion::Cache cache;
		mesher.attachCache(cache);
		const auto &volume = collection.at({-1, 0, 1}).volume();

		measure(name, 1, model.voxelCount(), expected.occupied, expected.vertices(), expected.triangles(), [&]() {
			return countMesh(mesher.bake(volume));
		});
	}

	void measureFourJoinedChunks()
	{
		constexpr std::int32_t Side = 16;
		using Chunk = spk::Voxel::Chunk<Side, Side, Side>;
		const auto model = makeModel({Side * 2, Side, Side * 2}, Pattern::TerracedCaves);
		const auto expected = countExposedFaces(model);
		const auto registry = makeRegistry();
		spk::World::ChunkCollection<Chunk> collection;

		for (std::uint32_t cz = 0; cz < 2; ++cz)
		{
			for (std::uint32_t cx = 0; cx < 2; ++cx)
			{
				Volume::Builder builder({Side, Side, Side});
				builder.bind(registry);
				for (std::uint32_t z = 0; z < Side; ++z)
				{
					for (std::uint32_t y = 0; y < Side; ++y)
					{
						for (std::uint32_t x = 0; x < Side; ++x)
						{
							const auto id = model.at(cx * Side + x, y, cz * Side + z);
							if (id != 0)
							{
								builder.set({x, y, z}, id);
							}
						}
					}
				}
				Chunk chunk;
				chunk.setVolume(builder.build());
				collection.addChunk({static_cast<std::int32_t>(cx) - 1, 0, static_cast<std::int32_t>(cz) - 1}, std::move(chunk));
			}
		}

		spk::Voxel::Occlusion::Cache cache;
		measure("Four joined 16^3 cave chunks", 4, model.voxelCount(), expected.occupied, expected.vertices(), expected.triangles(), [&]() {
			MeshCounts result;
			for (const auto &[coordinate, entry] : collection)
			{
				spk::Voxel::ChunkMesher<Side, Side, Side> mesher(collection, coordinate);
				mesher.attachCache(cache);
				const auto current = countMesh(mesher.bake(entry.chunk().volume()));
				result.vertices += current.vertices;
				result.opaqueVertices += current.opaqueVertices;
				result.transparentVertices += current.transparentVertices;
				result.triangles += current.triangles;
			}
			return result;
		});
	}

	[[nodiscard]] std::size_t isolatedShapeVertices(const spk::Voxel::Shape &shape)
	{
		const auto &instance = shape.instance({});
		const auto &polygons = instance.triangles();
		std::size_t result = 0;
		for (const auto &polygon : polygons)
		{
			if (polygon.vertices.size() >= 3)
			{
				result += (polygon.vertices.size() - 2) * 3;
			}
		}
		return result;
	}

	void measureMixedShapes()
	{
		constexpr std::int32_t Side = 16;
		using Chunk = spk::Voxel::Chunk<Side, Side, Side>;
		const auto registry = makeRegistry();
		const std::array<std::size_t, 4> perShapeVertices{
			isolatedShapeVertices(loadShape("cube")),
			isolatedShapeVertices(loadShape("slab")),
			isolatedShapeVertices(loadShape("slope")),
			isolatedShapeVertices(loadShape("stair"))};
		Volume::Builder builder({Side, Side, Side});
		builder.bind(registry);
		std::size_t expectedVertices = 0;
		std::size_t occupied = 0;
		for (std::uint32_t z = 0; z < Side; z += 3)
		{
			for (std::uint32_t y = 0; y < Side; y += 3)
			{
				for (std::uint32_t x = 0; x < Side; x += 3)
				{
					const std::size_t index = (x / 3 + y / 3 + z / 3) % perShapeVertices.size();
					builder.set({x, y, z}, static_cast<std::uint32_t>(index + 1 == 2 ? 3 : index + 1 == 3 ? 4 : index + 1 == 4 ? 5 : 1));
					expectedVertices += perShapeVertices[index];
					++occupied;
				}
			}
		}
		Chunk chunk;
		chunk.setVolume(builder.build());
		spk::World::ChunkCollection<Chunk> collection;
		collection.addChunk({0, 0, 0}, std::move(chunk));
		spk::Voxel::ChunkMesher<Side, Side, Side> mesher(collection, {0, 0, 0});
		spk::Voxel::Occlusion::Cache cache;
		mesher.attachCache(cache);
		measure("Isolated cube/slab/slope/stair mix", 1, Side * Side * Side, occupied, expectedVertices, expectedVertices / 3, [&]() {
			return countMesh(mesher.bake(collection.at({0, 0, 0}).volume()));
		});
	}
}

TEST(VoxelChunkPerformanceTest, Solid8)
{
	measureSingle<8, 8, 8>("Solid 8^3", Pattern::Solid);
}

TEST(VoxelChunkPerformanceTest, Solid16)
{
	measureSingle<16, 16, 16>("Solid 16^3", Pattern::Solid);
}

TEST(VoxelChunkPerformanceTest, Solid32)
{
	measureSingle<32, 32, 32>("Solid 32^3", Pattern::Solid);
}

TEST(VoxelChunkPerformanceTest, HollowBuilding32)
{
	measureSingle<32, 32, 32>("Hollow building with doors", Pattern::HollowRooms);
}

TEST(VoxelChunkPerformanceTest, TerracedCaves32)
{
	measureSingle<32, 32, 32>("Terraced terrain with caves", Pattern::TerracedCaves);
}

TEST(VoxelChunkPerformanceTest, BridgesAndTowers32)
{
	measureSingle<32, 32, 32>("Bridges and towers", Pattern::BridgesAndTowers);
}

TEST(VoxelChunkPerformanceTest, Checkerboard16)
{
	measureSingle<16, 16, 16>("3D checkerboard", Pattern::Checkerboard);
}

TEST(VoxelChunkPerformanceTest, NonCubicTerrain)
{
	measureSingle<8, 16, 24>("Non-cubic 8x16x24 cave terrain", Pattern::TerracedCaves);
}

TEST(VoxelChunkPerformanceTest, JoinedCaveChunks)
{
	measureFourJoinedChunks();
}

TEST(VoxelChunkPerformanceTest, MixedSlabSlopeStair)
{
	measureMixedShapes();
}
