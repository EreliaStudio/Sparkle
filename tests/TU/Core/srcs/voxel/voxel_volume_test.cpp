#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

#include "exception.hpp"
#include "voxel/detail/packed_cell.hpp"
#include "voxel/volume.hpp"

namespace
{
	thread_local bool FailNextVolumeAllocation = false;
}

void *operator new(std::size_t size)
{
	if (FailNextVolumeAllocation)
	{
		FailNextVolumeAllocation = false;
		throw std::bad_alloc();
	}
	if (void *memory = std::malloc(size == 0 ? 1 : size))
	{
		return memory;
	}
	throw std::bad_alloc();
}

void operator delete(void *memory) noexcept
{
	std::free(memory);
}

void operator delete(void *memory, std::size_t) noexcept
{
	std::free(memory);
}

namespace spk::Voxel
{
	struct VolumeTestAccess
	{
		[[nodiscard]] static std::size_t definitions(const Volume::Builder &builder)
		{
			return builder._definitions.size();
		}

		[[nodiscard]] static std::uint32_t encoded(const Volume::Builder &builder, std::size_t index)
		{
			return builder._cells.at(index);
		}

		[[nodiscard]] static const void *snapshotData(const Volume &volume)
		{
			return volume._data.get();
		}

		[[nodiscard]] static bool consumed(const Volume::Builder &builder)
		{
			return builder._consumed;
		}
	};
}

namespace
{
	using Volume = spk::Voxel::Volume;
	using Cell = spk::Voxel::Cell;
	using Definition = spk::Voxel::Definition;
	using Shape = spk::Voxel::Shape;
	using PackedCell = spk::Voxel::Detail::PackedCell;
	using Access = spk::Voxel::VolumeTestAccess;
	using Position = spk::Vector3UInt;

	static_assert(sizeof(PackedCell::Storage) == 4);
	static_assert(std::is_nothrow_copy_constructible_v<Volume>);
	static_assert(std::is_same_v<decltype(std::declval<const Volume &>().cell(Position{})), Cell>);

	Definition definition()
	{
		Shape::Builder shape;
		shape.addSlot("surface");
		return Definition(shape.build(), {{"surface", "stone"}});
	}

	constexpr std::array<Cell::HorizontalOrientation, 4> Horizontals{
		Cell::HorizontalOrientation::XPositive,
		Cell::HorizontalOrientation::ZNegative,
		Cell::HorizontalOrientation::XNegative,
		Cell::HorizontalOrientation::ZPositive};

	constexpr std::array<Cell::VerticalOrientation, 2> Verticals{
		Cell::VerticalOrientation::YPositive,
		Cell::VerticalOrientation::YNegative};

	TEST(VoxelVolumeTest, AllPositionsStartAsCanonicalAir)
	{
		Volume::Builder builder({3, 2, 4});
		EXPECT_EQ(Access::definitions(builder), 1u);
		const Volume volume = builder.build();
		EXPECT_EQ(volume.dimensions(), Position(3, 2, 4));
		for (std::uint32_t z = 0; z < 4; ++z)
		{
			for (std::uint32_t y = 0; y < 2; ++y)
			{
				for (std::uint32_t x = 0; x < 3; ++x)
				{
					const Cell cell = volume.cell({x, y, z});
					EXPECT_TRUE(cell.isAir());
					EXPECT_TRUE(cell.definition() == Definition::air());
					EXPECT_EQ(cell.horizontalOrientation(), Horizontals[0]);
					EXPECT_EQ(cell.verticalOrientation(), Verticals[0]);
				}
			}
		}
	}

	TEST(VoxelVolumeTest, DistinctPositionsAndBoundaryCellsDoNotAlias)
	{
		const Definition first = definition();
		const Definition second = definition();
		Volume::Builder builder({2, 3, 4});
		builder.set({0, 0, 0}, Cell(first));
		builder.set({1, 2, 3}, Cell(second, Horizontals[3], Verticals[1]));
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.cell({1, 2, 3}).definition() == second);
		EXPECT_EQ(volume.cell({1, 2, 3}).horizontalOrientation(), Horizontals[3]);
		EXPECT_TRUE(volume.cell({1, 0, 0}).isAir());
		EXPECT_TRUE(volume.cell({0, 2, 3}).isAir());
	}

	TEST(VoxelVolumeTest, EightOrientationsRoundTripForOneDefinition)
	{
		const Definition solid = definition();
		Volume::Builder builder({4, 2, 1});
		for (std::uint32_t i = 0; i < 8; ++i)
		{
			builder.set({i % 4, i / 4, 0}, Cell(solid, Horizontals[i % 4], Verticals[i / 4]));
		}
		EXPECT_EQ(Access::definitions(builder), 2u);
		for (std::size_t i = 0; i < 8; ++i)
		{
			const auto packed = Access::encoded(builder, i);
			EXPECT_EQ(PackedCell::definitionIndex(packed), 1u);
			EXPECT_EQ(PackedCell::horizontal(packed), Horizontals[i % 4]);
			EXPECT_EQ(PackedCell::vertical(packed), Verticals[i / 4]);
		}
		const Volume volume = builder.build();
		for (std::uint32_t i = 0; i < 8; ++i)
		{
			const Cell cell = volume.cell({i % 4, i / 4, 0});
			EXPECT_TRUE(cell.definition() == solid);
			EXPECT_EQ(cell.horizontalOrientation(), Horizontals[i % 4]);
			EXPECT_EQ(cell.verticalOrientation(), Verticals[i / 4]);
		}
	}

	TEST(VoxelVolumeTest, IdenticalDefinitionsDeduplicateOnlyWhenDataIdentityMatches)
	{
		const Definition first = definition();
		const Definition copy = first;
		const Definition independent = definition();
		EXPECT_TRUE(first == copy);
		EXPECT_FALSE(first == independent);
		Volume::Builder builder({3, 1, 1});
		builder.set({0, 0, 0}, Cell(first));
		builder.set({1, 0, 0}, Cell(copy));
		builder.set({2, 0, 0}, Cell(independent));
		EXPECT_EQ(Access::definitions(builder), 3u);
		EXPECT_EQ(PackedCell::definitionIndex(Access::encoded(builder, 0)), 1u);
		EXPECT_EQ(PackedCell::definitionIndex(Access::encoded(builder, 1)), 1u);
		EXPECT_EQ(PackedCell::definitionIndex(Access::encoded(builder, 2)), 2u);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.cell({1, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.cell({2, 0, 0}).definition() == independent);
	}

	TEST(VoxelVolumeTest, AirAlwaysUsesZeroIndexAndEraseResetsOrientation)
	{
		const Definition solid = definition();
		Volume::Builder builder({2, 1, 1});
		builder.set({0, 0, 0}, Cell(solid, Horizontals[2], Verticals[1]));
		builder.set({1, 0, 0}, Cell(Definition::air(), Horizontals[3], Verticals[1]));
		EXPECT_EQ(Access::definitions(builder), 2u);
		EXPECT_EQ(PackedCell::definitionIndex(Access::encoded(builder, 1)), 0u);
		builder.erase({0, 0, 0});
		EXPECT_EQ(Access::encoded(builder, 0), 0u);
		EXPECT_EQ(Access::definitions(builder), 2u);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).isAir());
		EXPECT_EQ(volume.cell({0, 0, 0}).horizontalOrientation(), Horizontals[0]);
		EXPECT_EQ(volume.cell({0, 0, 0}).verticalOrientation(), Verticals[0]);
		EXPECT_TRUE(volume.cell({1, 0, 0}).isAir());
		EXPECT_EQ(volume.cell({1, 0, 0}).horizontalOrientation(), Horizontals[3]);
		EXPECT_EQ(volume.cell({1, 0, 0}).verticalOrientation(), Verticals[1]);
	}

	TEST(VoxelVolumeTest, PublicCellIsRebuiltByValueWithPersistentDefinition)
	{
		const Definition solid = definition();
		Volume::Builder builder({1, 1, 1});
		builder.set({0, 0, 0}, Cell(solid, Horizontals[1], Verticals[1]));
		const Volume volume = builder.build();
		Cell detached = volume.cell({0, 0, 0});
		detached = Cell();
		EXPECT_TRUE(detached.isAir());
		const Cell again = volume.cell({0, 0, 0});
		EXPECT_TRUE(again.definition() == solid);
		EXPECT_EQ(again.horizontalOrientation(), Horizontals[1]);
		EXPECT_EQ(again.verticalOrientation(), Verticals[1]);
	}

	TEST(VoxelVolumeTest, RejectsOutOfBoundsWithoutModifyingBuilder)
	{
		const Definition solid = definition();
		Volume::Builder builder({2, 2, 2});
		builder.set({0, 0, 0}, Cell(solid));
		const auto before = Access::encoded(builder, 0);
		const auto definitionCount = Access::definitions(builder);
		EXPECT_THROW(builder.set({2, 0, 0}, Cell(definition())), spk::Exception);
		EXPECT_THROW(builder.set({0, 2, 0}, Cell(definition())), spk::Exception);
		EXPECT_THROW(builder.erase({0, 0, 2}), spk::Exception);
		EXPECT_EQ(Access::encoded(builder, 0), before);
		EXPECT_EQ(Access::definitions(builder), definitionCount);
		const Volume volume = builder.build();
		EXPECT_THROW((void)volume.cell({2, 0, 0}), spk::Exception);
		EXPECT_THROW((void)volume.cell({0, 2, 0}), spk::Exception);
		EXPECT_THROW((void)volume.cell({0, 0, 2}), spk::Exception);
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == solid);
	}

	TEST(VoxelVolumeTest, RejectsZeroAndOverflowingDimensionsBeforeAllocation)
	{
		using Limits = std::numeric_limits<std::uint32_t>;
		EXPECT_THROW((void)Volume::Builder(Position(0, 1, 1)), spk::Exception);
		EXPECT_THROW((void)Volume::Builder(Position(1, 0, 1)), spk::Exception);
		EXPECT_THROW((void)Volume::Builder(Position(1, 1, 0)), spk::Exception);
		EXPECT_THROW((void)Volume::Builder(Position(Limits::max(), Limits::max(), Limits::max())), spk::Exception);
		EXPECT_THROW((void)Volume::Builder(Position(Limits::max(), Limits::max(), 1)), spk::Exception);
	}

	TEST(VoxelVolumeTest, PackedEncodingChecksCapacityAndInvalidOrientations)
	{
		const auto maximum = PackedCell::MaxDefinitionIndex;
		const auto packed = PackedCell::encode(maximum, Horizontals[3], Verticals[1]);
		EXPECT_EQ(PackedCell::definitionIndex(packed), maximum);
		EXPECT_EQ(PackedCell::horizontal(packed), Horizontals[3]);
		EXPECT_EQ(PackedCell::vertical(packed), Verticals[1]);
		EXPECT_THROW((void)PackedCell::encode(std::size_t(maximum) + 1, Horizontals[0], Verticals[0]), spk::Exception);
		EXPECT_THROW((void)PackedCell::encode(0, static_cast<Cell::HorizontalOrientation>(45), Verticals[0]), spk::Exception);
		EXPECT_THROW((void)PackedCell::encode(0, Horizontals[0], static_cast<Cell::VerticalOrientation>(2)), spk::Exception);
		Volume::Builder builder({1, 1, 1});
		EXPECT_THROW(builder.set({0, 0, 0}, Cell(definition(), static_cast<Cell::HorizontalOrientation>(45))), spk::Exception);
		EXPECT_EQ(Access::definitions(builder), 1u);
		EXPECT_TRUE(builder.build().cell({0, 0, 0}).isAir());
	}

	TEST(VoxelVolumeTest, CopiesShareSnapshotAndBuildersNeverModifyPriorVolumes)
	{
		const Definition solid = definition();
		Volume::Builder builder({2, 1, 1});
		builder.set({0, 0, 0}, Cell(solid));
		const Volume original = builder.build();
		const Volume copy = original;
		EXPECT_EQ(Access::snapshotData(original), Access::snapshotData(copy));
		Volume::Builder changed(original);
		EXPECT_EQ(Access::definitions(changed), 2u);
		changed.erase({0, 0, 0});
		changed.set({1, 0, 0}, Cell(solid, Horizontals[2]));
		const Volume updated = changed.build();
		EXPECT_NE(Access::snapshotData(original), Access::snapshotData(updated));
		EXPECT_TRUE(original.cell({0, 0, 0}).definition() == solid);
		EXPECT_TRUE(copy.cell({0, 0, 0}).definition() == solid);
		EXPECT_TRUE(original.cell({1, 0, 0}).isAir());
		EXPECT_TRUE(updated.cell({0, 0, 0}).isAir());
		EXPECT_TRUE(updated.cell({1, 0, 0}).definition() == solid);
		EXPECT_EQ(updated.cell({1, 0, 0}).horizontalOrientation(), Horizontals[2]);
	}

	TEST(VoxelVolumeTest, SuccessfulBuildConsumesBuilder)
	{
		Volume::Builder builder({1, 1, 1});
		const Volume volume = builder.build();
		EXPECT_TRUE(Access::consumed(builder));
		EXPECT_THROW(builder.build(), spk::Exception);
		EXPECT_THROW(builder.erase({0, 0, 0}), spk::Exception);
		EXPECT_THROW(builder.set({0, 0, 0}, Cell()), spk::Exception);
		EXPECT_TRUE(volume.cell({0, 0, 0}).isAir());
	}

	TEST(VoxelVolumeTest, FailedBuildPreservesStateAndBuilderCanBeReused)
	{
		const Definition solid = definition();
		Volume::Builder builder({2, 1, 1});
		builder.set({1, 0, 0}, Cell(solid, Horizontals[2]));
		const auto expected = Access::encoded(builder, 1);
		const auto count = Access::definitions(builder);
		bool failed = false;
		FailNextVolumeAllocation = true;
		try
		{
			(void)builder.build();
		}
		catch (const std::bad_alloc &)
		{
			failed = true;
		}
		FailNextVolumeAllocation = false;
		ASSERT_TRUE(failed);
		EXPECT_FALSE(Access::consumed(builder));
		EXPECT_EQ(Access::encoded(builder, 1), expected);
		EXPECT_EQ(Access::definitions(builder), count);
		builder.set({0, 0, 0}, Cell(solid));
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == solid);
		EXPECT_EQ(volume.cell({1, 0, 0}).horizontalOrientation(), Horizontals[2]);
	}

	TEST(VoxelVolumeTest, SeparateVolumesUseIndependentIndexTables)
	{
		const Definition a = definition();
		const Definition b = definition();
		Volume::Builder first({2, 1, 1});
		Volume::Builder second({2, 1, 1});
		first.set({0, 0, 0}, Cell(a));
		first.set({1, 0, 0}, Cell(b));
		second.set({0, 0, 0}, Cell(b));
		second.set({1, 0, 0}, Cell(a));
		EXPECT_EQ(PackedCell::definitionIndex(Access::encoded(first, 0)), 1u);
		EXPECT_EQ(PackedCell::definitionIndex(Access::encoded(second, 0)), 1u);
		const Volume left = first.build();
		const Volume right = second.build();
		EXPECT_TRUE(left.cell({0, 0, 0}).definition() == a);
		EXPECT_TRUE(right.cell({0, 0, 0}).definition() == b);
		EXPECT_TRUE(left.cell({1, 0, 0}).definition() == b);
		EXPECT_TRUE(right.cell({1, 0, 0}).definition() == a);
	}

	TEST(VoxelVolumeTest, OneCellAndRepeatedReplacementsReuseDefinitions)
	{
		const Definition a = definition();
		const Definition b = definition();
		Volume::Builder builder({1, 1, 1});
		builder.set({0, 0, 0}, Cell(a));
		builder.set({0, 0, 0}, Cell(b));
		builder.set({0, 0, 0}, Cell(a, Horizontals[3]));
		EXPECT_EQ(Access::definitions(builder), 3u);
		EXPECT_EQ(PackedCell::definitionIndex(Access::encoded(builder, 0)), 1u);
		EXPECT_EQ(builder.build().cell({0, 0, 0}).horizontalOrientation(), Horizontals[3]);
	}
}
