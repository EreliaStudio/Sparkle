#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "math/perlin.hpp"
#include "world/voxel/biome_generator.hpp"
#include "world/voxel/chunk.hpp"

namespace
{
	using Chunk = spk::World::Voxel::Chunk<16, 16, 16>;
	using Coordinate = spk::World::ChunkCoordinate;
	using GeneratorBase = spk::World::Voxel::Biome::Generator<Chunk, 1>;

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

	class TerrainBiome final : public spk::World::Voxel::Biome
	{
	private:
		spk::Perlin _noise;
		bool _flat;

	public:
		TerrainBiome(std::uint32_t seed, bool flat) :
			_noise(seed),
			_flat(flat)
		{
			_noise.setOctaves(1);
			_noise.setFrequency(0.06f);

			Stratum surface;
			surface.materials = {{1, 1}};
			if (flat)
			{
				surface.materials.push_back({5, 1});
				surface.distribution = Stratum::Distribution::Custom{[](const spk::Vector2 &position) {
					return static_cast<std::int64_t>(position.x) % 2 == 0 ? 0.0f : 1.0f;
				}};
			}
			strata().push_back(std::move(surface));

			Stratum soil;
			soil.thickness = Stratum::Thickness::Fixed{flat ? 2u : 3u};
			soil.materials = {{2, 1}};
			strata().push_back(std::move(soil));
			if (flat)
			{
				Stratum extra;
				extra.materials = {{4, 1}};
				strata().push_back(std::move(extra));
			}
			Stratum deep;
			deep.thickness = Stratum::Thickness::Infinite{};
			deep.materials = {{3, 1}};
			strata().push_back(std::move(deep));
		}

		[[nodiscard]] float groundHeight(const spk::Vector2 &position) const override
		{
			return _flat ? 17.0f : static_cast<float>(std::floor(9.0 + 9.0 * _noise.sample2D(position.x, position.y, -1.0f, 1.0f)));
		}
	};

	class Generator final : public GeneratorBase
	{
	private:
		class TerrainDistribution final : public DistributionPass
		{
		private:
			TerrainBiome _biome;
			Distribution _function = [this](const spk::Vector2 &, Cell &cell) {
				cell[0] = {&_biome, 1.0f};
			};

		public:
			TerrainDistribution(std::uint32_t seed, bool flat) :
				_biome(seed, flat)
			{
				bind(&_function);
			}

			[[nodiscard]] const TerrainBiome &biome() const noexcept
			{
				return _biome;
			}
		};

		spk::Voxel::Definition::Registry _registry = registry();
		TerrainDistribution &_distribution;

	public:
		explicit Generator(std::uint32_t seed = 42, bool flat = false) :
			_distribution(addPass<TerrainDistribution>(seed, flat))
		{
			bind(&_registry);
			addPass<CombinationPass>(HeightCombination::Dominant);
			addPass<StrataGeneration>();
			addPass<Application>();
		}

		[[nodiscard]] std::int32_t height(std::int64_t x, std::int64_t z) const
		{
			return static_cast<std::int32_t>(_distribution.biome().groundHeight({static_cast<float>(x), static_cast<float>(z)}));
		}
	};

	[[nodiscard]] std::uint32_t idAt(const Chunk &chunk, std::uint32_t x, std::uint32_t y, std::uint32_t z)
	{
		return spk::Voxel::Volume::PackedCell::definitionId(chunk.volume().atRaw({x, y, z}));
	}

	TEST(PerlinBiomeTerrainTest, RepeatedGenerationAndOrderDoNotChangeRealChunks)
	{
		Generator generator;
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
		generator.flush();
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

	TEST(PerlinBiomeTerrainTest, AbsoluteCoordinatesAndNegativeChunkSeamsMatchHeightfield)
	{
		Generator generator;
		for (const Coordinate coordinate : {Coordinate{-2, 0, -1}, {-1, 0, 0}, {0, 0, 0}, {1, 0, 0}})
		{
			const auto chunk = generator.generate(coordinate);
			ASSERT_TRUE(chunk.has_value());
			const auto origin = Chunk::chunkCoordinateToWorld(coordinate);
			for (std::uint32_t z = 0; z < 16; ++z)
			{
				for (std::uint32_t x = 0; x < 16; ++x)
				{
					const std::int32_t top = generator.height(origin.x + x, origin.z + z);
					for (std::uint32_t y = 0; y < 16; ++y)
					{
						const std::int64_t worldY = origin.y + y;
						EXPECT_EQ(idAt(*chunk, x, y, z) != 0u, worldY <= top);
					}
				}
			}
		}
	}

	TEST(PerlinBiomeTerrainTest, VerticalChunksAndMaterialLayersAreClippedCorrectly)
	{
		Generator generator(42, true);
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

	TEST(PerlinBiomeTerrainTest, DistinctNoiseSeedsAffectHeightfield)
	{
		Generator first(123);
		Generator second(456);
		bool differs = false;
		for (int x = -43; x < 43; x += 7)
		{
			for (int z = -43; z < 43; z += 7)
			{
				differs = differs ||
						  first.height(x, z) != second.height(x, z);
			}
		}
		EXPECT_TRUE(differs);
	}
}
