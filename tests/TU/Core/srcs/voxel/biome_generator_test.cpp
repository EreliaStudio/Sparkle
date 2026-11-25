#include <gtest/gtest.h>

#include <atomic>
#include <future>
#include <limits>

#include "voxel/biome_generator.hpp"
#include "voxel/chunk.hpp"

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
			soil.minimumThickness = 2;
			soil.maximumThickness = 2;
			soil.materials = {{2, 1}};
			strata().push_back(soil);
			Stratum deep;
			deep.thicknessMode = Stratum::ThicknessMode::Infinite;
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

	protected:
		void _computeBiomePlacement(const spk::Vector2 &, Cell &cell) const override
		{
			++samples;
			cell[0] = {&_biome, 1};
		}

	public:
		mutable std::atomic<int> samples{0};

		Generator() :
			BiomeGenerator(registry())
		{
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
	generator.retain({-2, 0, -1});
	generator.retain({-2, 1, -1});
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
	generator.release({-2, 0, -1});
	generator.release({-2, 1, -1});
	ASSERT_TRUE(generator.generate({-2, 1, -1}).has_value());
	EXPECT_EQ(generator.samples.load(), 578);
}

TEST(VoxelBiomeGeneratorTest, FiniteStrataAndCustomMaterialSelection)
{
	Biome biome;
	biome.strata().resize(1);
	auto &stratum = biome.strata().front();
	stratum.minimumThickness = 2;
	stratum.maximumThickness = 2;
	stratum.materials = {{1, 1}, {4, 3}};
	stratum.distribution = Biome::Stratum::Distribution::Custom;
	stratum.customDistribution = [](const spk::Vector2 &position) {
		return position.x < 0 ? 0.0f : 1.0f;
	};
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
