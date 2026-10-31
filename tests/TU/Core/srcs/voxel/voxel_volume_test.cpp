#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include <utility>

#include "exception.hpp"
#include "voxel/volume.hpp"

namespace spk::Voxel
{
	struct VolumeTestAccess
	{
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
	using Registry = Definition::Registry;
	using Shape = spk::Voxel::Shape;
	using PackedCell = Volume::PackedCell;
	using Access = spk::Voxel::VolumeTestAccess;
	using Position = spk::Vector3UInt;

	static_assert(sizeof(PackedCell::Storage) == 4);
	static_assert(std::is_nothrow_copy_constructible_v<Volume>);
	static_assert(std::is_nothrow_copy_constructible_v<Registry>);
	static_assert(std::is_same_v<decltype(std::declval<const Volume &>().cell(Position{})), Cell>);

	Definition definition()
	{
		Shape::Builder builder;
		builder.addSlot("surface");
		return Definition(builder.build(), {{"surface", "stone"}});
	}

	Registry registry(std::initializer_list<std::pair<std::uint32_t, Definition>> entries)
	{
		Registry::Builder builder;
		for (const auto &[id, value] : entries)
		{
			builder.add(id, value);
		}
		return builder.build();
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
		Volume::Builder builder({2, 3, 4}, registry({{12, first}, {47, second}}));
		builder.set({0, 0, 0}, 12);
		builder.set({1, 2, 3}, 47, Horizontals[3], Verticals[1]);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.cell({1, 2, 3}).definition() == second);
		EXPECT_EQ(volume.cell({1, 2, 3}).horizontalOrientation(), Horizontals[3]);
		EXPECT_TRUE(volume.cell({1, 0, 0}).isAir());
		EXPECT_TRUE(volume.cell({0, 2, 3}).isAir());
	}

	TEST(VoxelVolumeTest, EightOrientationsRoundTripWithoutIDRemapping)
	{
		const Definition solid = definition();
		Volume::Builder builder({4, 2, 1}, registry({{400, solid}}));
		for (std::uint32_t i = 0; i < 8; ++i)
		{
			builder.set({i % 4, i / 4, 0}, 400, Horizontals[i % 4], Verticals[i / 4]);
		}
		for (std::size_t i = 0; i < 8; ++i)
		{
			const auto packed = Access::encoded(builder, i);
			EXPECT_EQ(PackedCell::definitionId(packed), 400u);
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

	TEST(VoxelVolumeTest, AliasedIDsRemainDistinctPackedIDs)
	{
		const Definition solid = definition();
		Volume::Builder builder({2, 1, 1}, registry({{7, solid}, {200, solid}}));
		builder.set({0, 0, 0}, 7);
		builder.set({1, 0, 0}, 200);
		EXPECT_EQ(PackedCell::definitionId(Access::encoded(builder, 0)), 7u);
		EXPECT_EQ(PackedCell::definitionId(Access::encoded(builder, 1)), 200u);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == solid);
		EXPECT_TRUE(volume.cell({1, 0, 0}).definition() == solid);
	}

	TEST(VoxelVolumeTest, AirAlwaysUsesIDZeroAndEraseResetsOrientation)
	{
		const Definition solid = definition();
		Volume::Builder builder({2, 1, 1}, registry({{27, solid}}));
		builder.set({0, 0, 0}, 27, Horizontals[2], Verticals[1]);
		builder.set({1, 0, 0}, 0, Horizontals[3], Verticals[1]);
		EXPECT_EQ(PackedCell::definitionId(Access::encoded(builder, 1)), 0u);
		builder.erase({0, 0, 0});
		EXPECT_EQ(Access::encoded(builder, 0), 0u);
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
		Volume::Builder builder({1, 1, 1}, registry({{101, solid}}));
		builder.set({0, 0, 0}, 101, Horizontals[1], Verticals[1]);
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
		Volume::Builder builder({2, 2, 2}, registry({{17, solid}}));
		builder.set({0, 0, 0}, 17);
		const auto before = Access::encoded(builder, 0);
		EXPECT_THROW(builder.set({2, 0, 0}, 17), spk::Exception);
		EXPECT_THROW(builder.set({0, 2, 0}, 17), spk::Exception);
		EXPECT_THROW(builder.erase({0, 0, 2}), spk::Exception);
		EXPECT_EQ(Access::encoded(builder, 0), before);
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
		const auto maximum = PackedCell::MaxDefinitionId;
		const auto packed = PackedCell::encode(maximum, Horizontals[3], Verticals[1]);
		EXPECT_EQ(PackedCell::definitionId(packed), maximum);
		EXPECT_EQ(PackedCell::horizontal(packed), Horizontals[3]);
		EXPECT_EQ(PackedCell::vertical(packed), Verticals[1]);
		EXPECT_THROW((void)PackedCell::encode(std::size_t(maximum) + 1, Horizontals[0], Verticals[0]), spk::Exception);
		EXPECT_THROW((void)PackedCell::encode(0, static_cast<Cell::HorizontalOrientation>(45), Verticals[0]), spk::Exception);
		EXPECT_THROW((void)PackedCell::encode(0, Horizontals[0], static_cast<Cell::VerticalOrientation>(2)), spk::Exception);
		Volume::Builder builder({1, 1, 1});
		EXPECT_THROW(builder.set({0, 0, 0}, 0, static_cast<Cell::HorizontalOrientation>(45)), spk::Exception);
		EXPECT_TRUE(builder.build().cell({0, 0, 0}).isAir());
	}

	TEST(VoxelVolumeTest, CopiesShareSnapshotAndBuildersNeverModifyPriorVolumes)
	{
		const Definition solid = definition();
		Volume::Builder builder({2, 1, 1}, registry({{72, solid}}));
		builder.set({0, 0, 0}, 72);
		const Volume original = builder.build();
		const Volume copy = original;
		EXPECT_EQ(Access::snapshotData(original), Access::snapshotData(copy));
		Volume::Builder changed(original);
		changed.erase({0, 0, 0});
		changed.set({1, 0, 0}, 72, Horizontals[2]);
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
		EXPECT_THROW(builder.set({0, 0, 0}, 0), spk::Exception);
		EXPECT_TRUE(volume.cell({0, 0, 0}).isAir());
	}

	TEST(VoxelVolumeTest, FailedSetPreservesAllEntriesAndBuilderRemainsReusable)
	{
		const Definition solid = definition();
		Volume::Builder builder({2, 1, 1}, registry({{54, solid}}));
		builder.set({1, 0, 0}, 54, Horizontals[2]);
		const auto before = Access::encoded(builder, 1);
		EXPECT_THROW(builder.set({0, 0, 0}, 99), spk::Exception);
		EXPECT_THROW(builder.set({0, 0, 0}, 54, static_cast<Cell::HorizontalOrientation>(360)), spk::Exception);
		EXPECT_THROW(builder.set({2, 0, 0}, 54), spk::Exception);
		EXPECT_FALSE(Access::consumed(builder));
		EXPECT_EQ(Access::encoded(builder, 1), before);
		builder.set({0, 0, 0}, 54);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == solid);
		EXPECT_EQ(volume.cell({1, 0, 0}).horizontalOrientation(), Horizontals[2]);
	}

	TEST(VoxelVolumeTest, SeparateVolumesPreserveExternalRegistryIDs)
	{
		const Definition a = definition();
		const Definition b = definition();
		const Registry definitions = registry({{50, a}, {8, b}});
		Volume::Builder first({2, 1, 1}, definitions);
		Volume::Builder second({2, 1, 1}, definitions);
		first.set({0, 0, 0}, 50);
		first.set({1, 0, 0}, 8);
		second.set({0, 0, 0}, 8);
		second.set({1, 0, 0}, 50);
		EXPECT_EQ(PackedCell::definitionId(Access::encoded(first, 0)), 50u);
		EXPECT_EQ(PackedCell::definitionId(Access::encoded(second, 0)), 8u);
		const Volume left = first.build();
		const Volume right = second.build();
		EXPECT_TRUE(left.cell({0, 0, 0}).definition() == a);
		EXPECT_TRUE(right.cell({0, 0, 0}).definition() == b);
		EXPECT_TRUE(left.cell({1, 0, 0}).definition() == b);
		EXPECT_TRUE(right.cell({1, 0, 0}).definition() == a);
	}

	TEST(VoxelVolumeTest, ReplacementsPreserveTheExactID)
	{
		const Definition a = definition();
		const Definition b = definition();
		Volume::Builder builder({1, 1, 1}, registry({{100, a}, {200, b}}));
		builder.set({0, 0, 0}, 100);
		builder.set({0, 0, 0}, 200);
		builder.set({0, 0, 0}, 100, Horizontals[3]);
		EXPECT_EQ(PackedCell::definitionId(Access::encoded(builder, 0)), 100u);
		EXPECT_EQ(builder.build().cell({0, 0, 0}).horizontalOrientation(), Horizontals[3]);
	}

	TEST(VoxelVolumeTest, SnapshotKeepsRegistryAliveAfterOriginalIsDestroyed)
	{
		const Definition solid = definition();
		Volume::Builder builder({1, 1, 1}, registry({{730, solid}}));
		builder.set({0, 0, 0}, 730);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == solid);
	}

	TEST(VoxelVolumeTest, RebindingRejectsChangedOrMissingUsedIDs)
	{
		const Definition solid = definition();
		const Definition other = definition();
		const Registry first = registry({{17, solid}});
		Volume::Builder builder({2, 1, 1}, first);
		builder.set({0, 0, 0}, 17);
		EXPECT_THROW(builder.bind(registry({{17, other}})), spk::Exception);
		EXPECT_THROW(builder.bind(Registry()), spk::Exception);
		builder.bind(registry({{17, solid}, {80, other}}));
		builder.set({1, 0, 0}, 80);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == solid);
		EXPECT_TRUE(volume.cell({1, 0, 0}).definition() == other);
	}
}
