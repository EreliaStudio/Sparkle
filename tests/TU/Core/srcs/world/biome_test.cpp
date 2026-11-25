#include <gtest/gtest.h>

#include <array>
#include <limits>
#include <type_traits>

#include "world/biome.hpp"
#include "world/pass/biome_distribution_pass.hpp"
#include "world/surface_biome.hpp"

namespace
{
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
		};
	};

	struct GenerationData
	{
		void flush()
		{
		}
	};

	using DistributionPass = spk::World::BiomeDistributionPass<Chunk, GenerationData>;
	using Influence = spk::World::SurfaceBiome::Influence;

	class TestPass : public DistributionPass
	{
	public:
		using DistributionPass::_normalize;

		void apply(const spk::World::ChunkCoordinate &, GenerationData &) override
		{
		}
	};

	static_assert(std::is_aggregate_v<Influence>);
	static_assert(std::is_copy_assignable_v<Influence>);
	static_assert(std::has_virtual_destructor_v<spk::World::Biome>);
	static_assert(std::is_abstract_v<spk::World::SurfaceBiome>);
	static_assert(std::is_abstract_v<DistributionPass>);
}

TEST(BiomeTest, SurfaceBiomeSuppliesAbsoluteContinuousHeight)
{
	ConstantBiome biome(24.5f);
	const spk::World::SurfaceBiome &surface = biome;
	EXPECT_FLOAT_EQ(surface.groundHeight({-12.0f, 8.0f}), 24.5f);

	const Influence influence;
	EXPECT_EQ(influence.biome, nullptr);
	EXPECT_FLOAT_EQ(influence.weight, 0.0f);
}

TEST(BiomeTest, NormalizationPreservesAllSlots)
{
	ConstantBiome forest(10.0f);
	ConstantBiome mountain(100.0f);
	ConstantBiome desert(20.0f);
	ConstantBiome plain(0.0f);
	std::array<Influence, 5> cell{{{&forest, 3.0f}, {}, {&mountain, 5.0f}, {&plain, 0.0f}, {&desert, 2.0f}}};
	TestPass::_normalize(cell);

	EXPECT_EQ(cell[0].biome, &forest);
	EXPECT_FLOAT_EQ(cell[0].weight, 0.3f);
	EXPECT_EQ(cell[1].biome, nullptr);
	EXPECT_FLOAT_EQ(cell[1].weight, 0.0f);
	EXPECT_EQ(cell[2].biome, &mountain);
	EXPECT_FLOAT_EQ(cell[2].weight, 0.5f);
	EXPECT_EQ(cell[3].biome, &plain);
	EXPECT_FLOAT_EQ(cell[3].weight, 0.0f);
	EXPECT_EQ(cell[4].biome, &desert);
	EXPECT_FLOAT_EQ(cell[4].weight, 0.2f);
}

TEST(BiomeTest, EmptyAndZeroSumCellsRemainUnchanged)
{
	ConstantBiome first(0.0f);
	ConstantBiome second(1.0f);
	std::array<Influence, 2> empty{};
	TestPass::_normalize(empty);
	for (const auto &influence : empty)
	{
		EXPECT_EQ(influence.biome, nullptr);
		EXPECT_FLOAT_EQ(influence.weight, 0.0f);
	}

	std::array<Influence, 3> zero{{{&first, 0.0f}, {}, {&second, 0.0f}}};
	TestPass::_normalize(zero);
	EXPECT_EQ(zero[0].biome, &first);
	EXPECT_FLOAT_EQ(zero[0].weight, 0.0f);
	EXPECT_EQ(zero[1].biome, nullptr);
	EXPECT_FLOAT_EQ(zero[1].weight, 0.0f);
	EXPECT_EQ(zero[2].biome, &second);
	EXPECT_FLOAT_EQ(zero[2].weight, 0.0f);
}

TEST(BiomeTest, SinglePositiveWeightBecomesOne)
{
	ConstantBiome biome(0.0f);
	std::array<Influence, 2> cell{{{}, {&biome, 3.0f}}};
	TestPass::_normalize(cell);
	EXPECT_EQ(cell[1].biome, &biome);
	EXPECT_FLOAT_EQ(cell[1].weight, 1.0f);
}

TEST(BiomeTest, NormalizationAccumulatesInDouble)
{
	ConstantBiome first(0.0f);
	ConstantBiome second(0.0f);
	const float maximum = std::numeric_limits<float>::max();
	std::array<Influence, 2> cell{{{&first, maximum}, {&second, maximum}}};
	TestPass::_normalize(cell);
	EXPECT_FLOAT_EQ(cell[0].weight, 0.5f);
	EXPECT_FLOAT_EQ(cell[1].weight, 0.5f);
}

TEST(BiomeTest, RejectsInvalidWeightsBeforeModifyingTheCell)
{
	ConstantBiome first(0.0f);
	ConstantBiome second(0.0f);
	const float infinity = std::numeric_limits<float>::infinity();
	const float nan = std::numeric_limits<float>::quiet_NaN();
	for (const Influence invalid : std::array<Influence, 7>{
			 Influence{nullptr, 1.0f}, Influence{nullptr, -1.0f}, Influence{nullptr, infinity}, Influence{nullptr, nan}, Influence{&second, -1.0f}, Influence{&second, infinity}, Influence{&second, nan}})
	{
		std::array<Influence, 2> cell{{{&first, 3.0f}, invalid}};
		EXPECT_THROW(TestPass::_normalize(cell), spk::Exception);
		EXPECT_FLOAT_EQ(cell[0].weight, 3.0f);
		EXPECT_EQ(cell[0].biome, &first);
	}
}

TEST(BiomeTest, RejectsDuplicateBiomeEvenAtZeroWeight)
{
	ConstantBiome biome(0.0f);
	std::array<Influence, 3> cell{{{&biome, 0.0f}, {}, {&biome, 0.0f}}};
	EXPECT_THROW(TestPass::_normalize(cell), spk::Exception);

	cell[0].weight = 3.0f;
	EXPECT_THROW(TestPass::_normalize(cell), spk::Exception);
	EXPECT_FLOAT_EQ(cell[0].weight, 3.0f);
}
