#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "math/perlin.hpp"
#include "voxel/chunk.hpp"
#include "voxel/definition.hpp"
#include "voxel/pass/perlin_terrain_pass.hpp"
#include "voxel/volume.hpp"
#include "world/generator.hpp"

namespace
{
	using Chunk = spk::Voxel::Chunk<16, 16, 16>;
	using Builder = spk::Voxel::Volume::Builder;
	using Coordinate = spk::World::ChunkCoordinate;

	struct GenerationData
	{
	};

	using GeneratorBase = spk::World::Generator<Chunk, GenerationData, Builder>;
	using TerrainPass = spk::Voxel::PerlinTerrainPass<Chunk, GenerationData>;

	[[nodiscard]] spk::Voxel::Definition::Registry registry()
	{
		spk::Voxel::Shape::Builder shape;
		shape.addSlot("surface");
		const auto definition = spk::Voxel::Definition(shape.build(), {{"surface", {"terrain"}}});
		spk::Voxel::Definition::Registry::Builder builder;
		for (std::uint32_t id = 1; id <= 5; ++id)
		{
			builder.add(id, definition);
		}
		return builder.build();
	}

	class Generator final : public GeneratorBase
	{
	private:
		spk::Voxel::Definition::Registry _registry = registry();

		[[nodiscard]] std::optional<Builder> _createChunkBuilder(const Coordinate &coordinate) override
		{
			(void)coordinate;
			Builder builder({16, 16, 16});
			builder.bind(_registry);
			return builder;
		}

		[[nodiscard]] GenerationData _createGenerationData() override
		{
			return {};
		}

	public:
		TerrainPass &terrain;

		Generator() :
			terrain(addPass<TerrainPass>())
		{
			terrain.setHeightRange(-32, 32);
			terrain.setBaseHeight(9.0f);
			terrain.setSurfaceVoxel(1);
			terrain.setSubsurfaceLayers({{2, 3}});
			terrain.setDeepVoxel(3);
		}

		void addNoise(std::uint32_t seed = 42)
		{
			spk::Perlin noise(seed);
			noise.setOctaves(1);
			noise.setFrequency(0.06f);
			terrain.addNoiseLayer(std::move(noise), 9.0f);
		}
	};

	[[nodiscard]] std::uint32_t idAt(const Chunk &chunk, std::uint32_t x, std::uint32_t y, std::uint32_t z)
	{
		return spk::Voxel::Volume::PackedCell::definitionId(chunk.volume().atRaw({x, y, z}));
	}

	TEST(PerlinTerrainPassTest, RepeatedGenerationAndOrderDoNotChangeRealChunks)
	{
		Generator generator;
		generator.addNoise();
		const std::array<Coordinate, 5> coords{
			Coordinate{-2, 0, 1}, {-1, 0, 0}, {0, 0, 0}, {1, 0, 0}, {2, 0, -1}};
		std::array<std::array<std::uint32_t, 16>, coords.size()> expected{};
		for (std::size_t i = 0; i < coords.size(); ++i)
		{
			const auto chunk = generator.generate(coords[i]);
			ASSERT_TRUE(chunk.has_value());
			for (std::uint32_t z = 0; z < 16; ++z)
			{
				expected[i][z] = idAt(*chunk, 7, 8, z);
			}
		}
		for (std::size_t i = coords.size(); i > 0; --i)
		{
			const auto chunk = generator.generate(coords[i - 1]);
			ASSERT_TRUE(chunk.has_value());
			for (std::uint32_t z = 0; z < 16; ++z)
			{
				EXPECT_EQ(idAt(*chunk, 7, 8, z), expected[i - 1][z]);
			}
		}
	}

	TEST(PerlinTerrainPassTest, AbsoluteCoordinatesAndNegativeChunkSeamsMatchHeightfield)
	{
		Generator generator;
		generator.addNoise();
		for (const Coordinate coordinate : {Coordinate{-2, 0, -1}, {-1, 0, 0}, {0, 0, 0}, {1, 0, 0}})
		{
			const auto chunk = generator.generate(coordinate);
			ASSERT_TRUE(chunk.has_value());
			const auto origin = Chunk::chunkCoordinateToWorld(coordinate);
			for (std::uint32_t z = 0; z < 16; ++z)
			{
				for (std::uint32_t x = 0; x < 16; ++x)
				{
					const std::int32_t top = generator.terrain.computeHeight(origin.x + x, origin.z + z);
					for (std::uint32_t y = 0; y < 16; ++y)
					{
						const std::int64_t worldY = origin.y + y;
						EXPECT_EQ(idAt(*chunk, x, y, z) != 0u, worldY <= top);
					}
				}
			}
		}
	}

	TEST(PerlinTerrainPassTest, VerticalChunksAndMaterialLayersAreClippedCorrectly)
	{
		Generator generator;
		generator.terrain.setHeightRange(17, 17);
		generator.terrain.setBaseHeight(17);
		generator.terrain.setSubsurfaceLayers({{2, 2}, {4, 1}});
		generator.terrain.setSurfaceSelector([](std::int64_t x, std::int64_t, std::int32_t) {
			return x % 2 == 0 ? 1u : 5u;
		});
		const auto below = generator.generate({0, -1, 0});
		const auto middle = generator.generate({0, 0, 0});
		const auto above = generator.generate({0, 1, 0});
		ASSERT_TRUE(below.has_value());
		ASSERT_TRUE(middle.has_value());
		ASSERT_TRUE(above.has_value());
		EXPECT_EQ(idAt(*below, 0, 15, 0), 3u);
		EXPECT_EQ(idAt(*middle, 0, 0, 0), 3u);
		EXPECT_EQ(idAt(*middle, 0, 14, 0), 4u);
		EXPECT_EQ(idAt(*middle, 0, 15, 0), 2u);
		EXPECT_EQ(idAt(*above, 0, 0, 0), 2u);
		EXPECT_EQ(idAt(*above, 0, 1, 0), 1u);
		EXPECT_EQ(idAt(*above, 1, 1, 0), 5u);
		EXPECT_EQ(idAt(*above, 0, 2, 0), 0u);
	}

	TEST(PerlinTerrainPassTest, ConfigurationsChangeElevationWithoutReconstruction)
	{
		Generator generator;
		generator.terrain.setHeightRange(-100, 100);
		generator.terrain.setBaseHeight(8.0f);
		EXPECT_EQ(generator.terrain.computeHeight(-12, 21), 8);
		generator.terrain.setBaseHeight(19.0f);
		EXPECT_EQ(generator.terrain.computeHeight(-12, 21), 19);
		generator.addNoise(7);
		const auto first = generator.terrain.computeHeight(23, 37);
		generator.terrain.clearNoiseLayers();
		EXPECT_EQ(generator.terrain.computeHeight(23, 37), 19);
		generator.addNoise(7);
		EXPECT_EQ(generator.terrain.computeHeight(23, 37), first);
		generator.terrain.setHeightRange(3, 3);
		EXPECT_EQ(generator.terrain.computeHeight(23, 37), 3);
	}

	TEST(PerlinTerrainPassTest, InvalidConfigurationFailsBeforeGeneration)
	{
		Generator generator;
		EXPECT_THROW(generator.terrain.setHeightRange(10, 9), spk::Exception);
		EXPECT_THROW(generator.terrain.setBaseHeight(std::numeric_limits<float>::infinity()), spk::Exception);
		EXPECT_THROW(generator.terrain.setSubsurfaceLayers({{2, 0}}), spk::Exception);
		EXPECT_THROW(generator.terrain.addNoiseLayer(spk::Perlin(1), std::numeric_limits<float>::quiet_NaN()), spk::Exception);
	}

	TEST(PerlinTerrainPassTest, DistinctNoiseSeedsAffectHeightfield)
	{
		Generator first;
		Generator second;
		first.addNoise(123);
		second.addNoise(456);
		bool differs = false;
		for (int x = -43; x < 43; x += 7)
		{
			for (int z = -43; z < 43; z += 7)
			{
				differs = differs ||
						  first.terrain.computeHeight(x, z) != second.terrain.computeHeight(x, z);
			}
		}
		EXPECT_TRUE(differs);
	}
}
