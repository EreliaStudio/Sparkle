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

namespace
{
	using Volume = spk::Voxel::Volume;
	using Cell = spk::Voxel::Cell;
	using Definition = spk::Voxel::Definition;
	using Registry = Definition::Registry;
	using Shape = spk::Voxel::Shape;
	using PackedCell = Volume::PackedCell;
	using Position = spk::Vector3UInt;

	static_assert(sizeof(PackedCell::Storage) == 4);
	static_assert(std::is_nothrow_copy_constructible_v<Volume>);
	static_assert(std::is_nothrow_copy_constructible_v<Registry>);
	static_assert(std::is_same_v<decltype(std::declval<const Volume &>().at(Position{})), Cell>);
	static_assert(std::is_same_v<decltype(std::declval<const Volume &>()[Position{}]), Cell>);
	static_assert(std::is_same_v<decltype(std::declval<const Volume &>().atRaw(Position{})), PackedCell::Storage>);

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

	TEST(VoxelVolumeTest, DefaultBuilderRequiresDimensionsBeforeOperations)
	{
		Volume::Builder builder;
		EXPECT_THROW(builder.build(), spk::Exception);
		EXPECT_THROW(builder.set({0, 0, 0}, 0), spk::Exception);
		EXPECT_THROW(builder.erase({0, 0, 0}), spk::Exception);
		builder.bind(Registry());
		builder.setDimensions({1, 2, 1});
		EXPECT_THROW(builder.setDimensions({1, 2, 1}), spk::Exception);
		EXPECT_EQ(builder.build().dimensions(), Position(1, 2, 1));
	}

	TEST(VoxelVolumeTest, LoadReplacesBuilderStateFromExistingVolume)
	{
		const Definition solid = definition();
		Volume::Builder originalBuilder;
		originalBuilder.setDimensions({2, 1, 1});
		originalBuilder.bind(registry({{8, solid}}));
		originalBuilder.set({1, 0, 0}, 8);
		const Volume source = originalBuilder.build();

		Volume::Builder builder;
		builder.setDimensions({1, 1, 1});
		builder.load(source);
		EXPECT_THROW(builder.setDimensions({2, 1, 1}), spk::Exception);
		const Volume restored = builder.build();
		EXPECT_EQ(restored.dimensions(), Position(2, 1, 1));
		EXPECT_EQ(restored.atRaw({1, 0, 0}), source.atRaw({1, 0, 0}));
		EXPECT_TRUE(restored.at({1, 0, 0}).definition() == solid);
	}

	TEST(VoxelVolumeTest, AllPositionsStartAsCanonicalAir)
	{
		Volume::Builder builder;
		builder.setDimensions({3, 2, 4});
		const Volume volume = builder.build();
		EXPECT_EQ(volume.dimensions(), Position(3, 2, 4));
		for (std::uint32_t z = 0; z < 4; ++z)
		{
			for (std::uint32_t y = 0; y < 2; ++y)
			{
				for (std::uint32_t x = 0; x < 3; ++x)
				{
					const Cell cell = volume.at({x, y, z});
					EXPECT_TRUE(cell.isAir());
					EXPECT_TRUE(cell.definition() == Definition::air());
					EXPECT_EQ(cell.horizontalOrientation(), Horizontals[0]);
					EXPECT_EQ(cell.verticalOrientation(), Verticals[0]);
				}
			}
		}
	}

	TEST(VoxelVolumeTest, PublicAccessorsReturnEquivalentCellsAndExactRawWord)
	{
		const Definition solid = definition();
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		builder.bind(registry({{42, solid}}));
		builder.set({1, 0, 0}, 42, Horizontals[2], Verticals[1]);
		const Volume volume = builder.build();
		const Position position{1, 0, 0};
		const Cell fromAt = volume.at(position);
		const Cell fromIndex = volume[position];
		EXPECT_TRUE(fromAt.definition() == fromIndex.definition());
		EXPECT_TRUE(fromAt.definition() == solid);
		EXPECT_EQ(fromAt.horizontalOrientation(), fromIndex.horizontalOrientation());
		EXPECT_EQ(fromAt.verticalOrientation(), fromIndex.verticalOrientation());
		EXPECT_EQ(volume.atRaw(position), PackedCell::encode(42, Horizontals[2], Verticals[1]));
		EXPECT_EQ(volume.atRaw({0, 0, 0}), PackedCell::Air);
	}

	TEST(VoxelVolumeTest, AllPublicAccessorsRejectOutOfBounds)
	{
		Volume::Builder builder;
		builder.setDimensions({1, 1, 1});
		const Volume volume = builder.build();
		const Position outside{1, 0, 0};
		EXPECT_THROW((void)volume.at(outside), spk::Exception);
		EXPECT_THROW((void)volume[outside], spk::Exception);
		EXPECT_THROW((void)volume.atRaw(outside), spk::Exception);
	}

	TEST(VoxelVolumeTest, DistinctPositionsAndBoundaryCellsDoNotAlias)
	{
		const Definition first = definition();
		const Definition second = definition();
		Volume::Builder builder;
		builder.setDimensions({2, 3, 4});
		builder.bind(registry({{12, first}, {47, second}}));
		builder.set({0, 0, 0}, 12);
		builder.set({1, 2, 3}, 47, Horizontals[3], Verticals[1]);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.at({1, 2, 3}).definition() == second);
		EXPECT_EQ(volume.at({1, 2, 3}).horizontalOrientation(), Horizontals[3]);
		EXPECT_TRUE(volume.at({1, 0, 0}).isAir());
		EXPECT_TRUE(volume.at({0, 2, 3}).isAir());
	}

	TEST(VoxelVolumeTest, EightOrientationsRoundTripWithoutIDRemapping)
	{
		const Definition solid = definition();
		Volume::Builder builder;
		builder.setDimensions({4, 2, 1});
		builder.bind(registry({{400, solid}}));
		for (std::uint32_t i = 0; i < 8; ++i)
		{
			builder.set({i % 4, i / 4, 0}, 400, Horizontals[i % 4], Verticals[i / 4]);
		}
		const Volume volume = builder.build();
		for (std::uint32_t i = 0; i < 8; ++i)
		{
			const auto packed = volume.atRaw({i % 4, i / 4, 0});
			EXPECT_EQ(PackedCell::definitionId(packed), 400u);
			EXPECT_EQ(PackedCell::horizontal(packed), Horizontals[i % 4]);
			EXPECT_EQ(PackedCell::vertical(packed), Verticals[i / 4]);
		}
		for (std::uint32_t i = 0; i < 8; ++i)
		{
			const Cell cell = volume.at({i % 4, i / 4, 0});
			EXPECT_TRUE(cell.definition() == solid);
			EXPECT_EQ(cell.horizontalOrientation(), Horizontals[i % 4]);
			EXPECT_EQ(cell.verticalOrientation(), Verticals[i / 4]);
		}
	}

	TEST(VoxelVolumeTest, AliasedIDsRemainDistinctPackedIDs)
	{
		const Definition solid = definition();
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		builder.bind(registry({{7, solid}, {200, solid}}));
		builder.set({0, 0, 0}, 7);
		builder.set({1, 0, 0}, 200);
		const Volume volume = builder.build();
		EXPECT_EQ(PackedCell::definitionId(volume.atRaw({0, 0, 0})), 7u);
		EXPECT_EQ(PackedCell::definitionId(volume.atRaw({1, 0, 0})), 200u);
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == solid);
		EXPECT_TRUE(volume.at({1, 0, 0}).definition() == solid);
	}

	TEST(VoxelVolumeTest, AirAlwaysUsesIDZeroAndEraseResetsOrientation)
	{
		const Definition solid = definition();
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		builder.bind(registry({{27, solid}}));
		builder.set({0, 0, 0}, 27, Horizontals[2], Verticals[1]);
		builder.set({1, 0, 0}, 0, Horizontals[3], Verticals[1]);
		builder.erase({0, 0, 0});
		const Volume volume = builder.build();
		EXPECT_EQ(PackedCell::definitionId(volume.atRaw({1, 0, 0})), 0u);
		EXPECT_EQ(volume.atRaw({0, 0, 0}), PackedCell::Air);
		EXPECT_TRUE(volume.at({0, 0, 0}).isAir());
		EXPECT_EQ(volume.at({0, 0, 0}).horizontalOrientation(), Horizontals[0]);
		EXPECT_EQ(volume.at({0, 0, 0}).verticalOrientation(), Verticals[0]);
		EXPECT_TRUE(volume.at({1, 0, 0}).isAir());
		EXPECT_EQ(volume.at({1, 0, 0}).horizontalOrientation(), Horizontals[3]);
		EXPECT_EQ(volume.at({1, 0, 0}).verticalOrientation(), Verticals[1]);
	}

	TEST(VoxelVolumeTest, PublicCellIsRebuiltByValueWithPersistentDefinition)
	{
		const Definition solid = definition();
		Volume::Builder builder;
		builder.setDimensions({1, 1, 1});
		builder.bind(registry({{101, solid}}));
		builder.set({0, 0, 0}, 101, Horizontals[1], Verticals[1]);
		const Volume volume = builder.build();
		Cell detached = volume.at({0, 0, 0});
		detached = Cell();
		EXPECT_TRUE(detached.isAir());
		const Cell again = volume.at({0, 0, 0});
		EXPECT_TRUE(again.definition() == solid);
		EXPECT_EQ(again.horizontalOrientation(), Horizontals[1]);
		EXPECT_EQ(again.verticalOrientation(), Verticals[1]);
	}

	TEST(VoxelVolumeTest, RejectsOutOfBoundsWithoutModifyingBuilder)
	{
		const Definition solid = definition();
		Volume::Builder builder;
		builder.setDimensions({2, 2, 2});
		builder.bind(registry({{17, solid}}));
		builder.set({0, 0, 0}, 17);
		const auto before = Volume::Builder(builder).build().atRaw({0, 0, 0});
		EXPECT_THROW(builder.set({2, 0, 0}, 17), spk::Exception);
		EXPECT_THROW(builder.set({0, 2, 0}, 17), spk::Exception);
		EXPECT_THROW(builder.erase({0, 0, 2}), spk::Exception);
		EXPECT_EQ(Volume::Builder(builder).build().atRaw({0, 0, 0}), before);
		const Volume volume = builder.build();
		EXPECT_THROW((void)volume.at({2, 0, 0}), spk::Exception);
		EXPECT_THROW((void)volume.at({0, 2, 0}), spk::Exception);
		EXPECT_THROW((void)volume.at({0, 0, 2}), spk::Exception);
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == solid);
	}

	TEST(VoxelVolumeTest, RejectsZeroAndOverflowingDimensionsBeforeAllocation)
	{
		using Limits = std::numeric_limits<std::uint32_t>;
		Volume::Builder builder;
		EXPECT_THROW(builder.setDimensions(Position(0, 1, 1)), spk::Exception);
		EXPECT_THROW(builder.setDimensions(Position(1, 0, 1)), spk::Exception);
		EXPECT_THROW(builder.setDimensions(Position(1, 1, 0)), spk::Exception);
		EXPECT_THROW(builder.setDimensions(Position(Limits::max(), Limits::max(), Limits::max())), spk::Exception);
		EXPECT_THROW(builder.setDimensions(Position(Limits::max(), Limits::max(), 1)), spk::Exception);
		builder.setDimensions({1, 1, 1});
		EXPECT_TRUE(builder.build().at({0, 0, 0}).isAir());
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
		Volume::Builder builder;
		builder.setDimensions({1, 1, 1});
		EXPECT_THROW(builder.set({0, 0, 0}, 0, static_cast<Cell::HorizontalOrientation>(45)), spk::Exception);
		EXPECT_TRUE(builder.build().at({0, 0, 0}).isAir());
	}

	TEST(VoxelVolumeTest, CopiesShareSnapshotAndBuildersNeverModifyPriorVolumes)
	{
		const Definition solid = definition();
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		builder.bind(registry({{72, solid}}));
		builder.set({0, 0, 0}, 72);
		const Volume original = builder.build();
		const Volume copy = original;
		EXPECT_EQ(original.atRaw({0, 0, 0}), copy.atRaw({0, 0, 0}));
		Volume::Builder changed;
		changed.load(original);
		changed.erase({0, 0, 0});
		changed.set({1, 0, 0}, 72, Horizontals[2]);
		const Volume updated = changed.build();
		EXPECT_NE(original.atRaw({0, 0, 0}), updated.atRaw({0, 0, 0}));
		EXPECT_TRUE(original.at({0, 0, 0}).definition() == solid);
		EXPECT_TRUE(copy.at({0, 0, 0}).definition() == solid);
		EXPECT_TRUE(original.at({1, 0, 0}).isAir());
		EXPECT_TRUE(updated.at({0, 0, 0}).isAir());
		EXPECT_TRUE(updated.at({1, 0, 0}).definition() == solid);
		EXPECT_EQ(updated.at({1, 0, 0}).horizontalOrientation(), Horizontals[2]);
	}

	TEST(VoxelVolumeTest, SuccessfulBuildConsumesBuilder)
	{
		Volume::Builder builder;
		builder.setDimensions({1, 1, 1});
		const Volume volume = builder.build();
		EXPECT_THROW(builder.build(), spk::Exception);
		EXPECT_THROW(builder.setDimensions({1, 1, 1}), spk::Exception);
		EXPECT_THROW(builder.load(volume), spk::Exception);
		EXPECT_THROW(builder.erase({0, 0, 0}), spk::Exception);
		EXPECT_THROW(builder.set({0, 0, 0}, 0), spk::Exception);
		EXPECT_TRUE(volume.at({0, 0, 0}).isAir());
	}

	TEST(VoxelVolumeTest, FailedSetPreservesAllEntriesAndBuilderRemainsReusable)
	{
		const Definition solid = definition();
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		builder.bind(registry({{54, solid}}));
		builder.set({1, 0, 0}, 54, Horizontals[2]);
		const auto before = Volume::Builder(builder).build().atRaw({1, 0, 0});
		EXPECT_THROW(builder.set({0, 0, 0}, 99), spk::Exception);
		EXPECT_THROW(builder.set({0, 0, 0}, 54, static_cast<Cell::HorizontalOrientation>(360)), spk::Exception);
		EXPECT_THROW(builder.set({2, 0, 0}, 54), spk::Exception);
		EXPECT_EQ(Volume::Builder(builder).build().atRaw({1, 0, 0}), before);
		builder.set({0, 0, 0}, 54);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == solid);
		EXPECT_EQ(volume.at({1, 0, 0}).horizontalOrientation(), Horizontals[2]);
	}

	TEST(VoxelVolumeTest, SeparateVolumesPreserveExternalRegistryIDs)
	{
		const Definition a = definition();
		const Definition b = definition();
		const Registry definitions = registry({{50, a}, {8, b}});
		Volume::Builder first;
		first.setDimensions({2, 1, 1});
		first.bind(definitions);
		Volume::Builder second;
		second.setDimensions({2, 1, 1});
		second.bind(definitions);
		first.set({0, 0, 0}, 50);
		first.set({1, 0, 0}, 8);
		second.set({0, 0, 0}, 8);
		second.set({1, 0, 0}, 50);
		const Volume left = first.build();
		const Volume right = second.build();
		EXPECT_EQ(PackedCell::definitionId(left.atRaw({0, 0, 0})), 50u);
		EXPECT_EQ(PackedCell::definitionId(right.atRaw({0, 0, 0})), 8u);
		EXPECT_TRUE(left.at({0, 0, 0}).definition() == a);
		EXPECT_TRUE(right.at({0, 0, 0}).definition() == b);
		EXPECT_TRUE(left.at({1, 0, 0}).definition() == b);
		EXPECT_TRUE(right.at({1, 0, 0}).definition() == a);
	}

	TEST(VoxelVolumeTest, ReplacementsPreserveTheExactID)
	{
		const Definition a = definition();
		const Definition b = definition();
		Volume::Builder builder;
		builder.setDimensions({1, 1, 1});
		builder.bind(registry({{100, a}, {200, b}}));
		builder.set({0, 0, 0}, 100);
		builder.set({0, 0, 0}, 200);
		builder.set({0, 0, 0}, 100, Horizontals[3]);
		const Volume volume = builder.build();
		EXPECT_EQ(PackedCell::definitionId(volume.atRaw({0, 0, 0})), 100u);
		EXPECT_EQ(volume.at({0, 0, 0}).horizontalOrientation(), Horizontals[3]);
	}

	TEST(VoxelVolumeTest, SnapshotKeepsRegistryAliveAfterOriginalIsDestroyed)
	{
		const Definition solid = definition();
		Volume::Builder builder;
		builder.setDimensions({1, 1, 1});
		builder.bind(registry({{730, solid}}));
		builder.set({0, 0, 0}, 730);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == solid);
	}

	TEST(VoxelVolumeTest, RebindingRejectsChangedOrMissingUsedIDs)
	{
		const Definition solid = definition();
		const Definition other = definition();
		const Registry first = registry({{17, solid}});
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		builder.bind(first);
		builder.set({0, 0, 0}, 17);
		EXPECT_THROW(builder.bind(registry({{17, other}})), spk::Exception);
		EXPECT_THROW(builder.bind(Registry()), spk::Exception);
		builder.bind(registry({{17, solid}, {80, other}}));
		builder.set({1, 0, 0}, 80);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == solid);
		EXPECT_TRUE(volume.at({1, 0, 0}).definition() == other);
	}
}
