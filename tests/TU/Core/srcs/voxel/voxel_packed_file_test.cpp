#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "voxel/volume.hpp"

namespace
{
	using Volume = spk::Voxel::Volume;
	using Cell = spk::Voxel::Cell;
	using Shape = spk::Voxel::Shape;
	using Definition = spk::Voxel::Definition;
	using Collection = Definition::Collection;
	using Packed = Volume::PackedCell;
	using Position = spk::Vector3UInt;

	Definition createDefinition(std::string semantic = "stone")
	{
		Shape::Builder builder;
		builder.addSlot("face");
		return Definition(builder.build(), {{"face", {std::move(semantic)}}});
	}

	Collection makeCollection(std::initializer_list<std::pair<std::uint32_t, Definition>> entries)
	{
		Collection::Builder builder;
		for (const auto &[id, value] : entries)
		{
			builder.add(id, value);
		}
		return builder.build();
	}

	struct PackedFile
	{
		std::filesystem::path path;

		PackedFile()
		{
			static std::atomic<std::uint64_t> number = 0;
			const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
			path = std::filesystem::temp_directory_path() /
				   ("sparkle-voxel-" + std::to_string(ticks) + "-" + std::to_string(++number) + ".bin");
		}

		~PackedFile()
		{
			std::error_code error;
			std::filesystem::remove(path, error);
		}

		void write(const std::vector<std::uint32_t> &cells) const
		{
			std::ofstream output(path, std::ios::binary | std::ios::trunc);
			if (!output)
			{
				throw std::runtime_error("Could not create temporary packed voxel file");
			}
			for (const auto cell : cells)
			{
				const std::array<unsigned char, 4> bytes{
					static_cast<unsigned char>(cell),
					static_cast<unsigned char>(cell >> 8),
					static_cast<unsigned char>(cell >> 16),
					static_cast<unsigned char>(cell >> 24)};
				output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
			}
			if (!output)
			{
				throw std::runtime_error("Could not write temporary packed voxel file");
			}
		}
	};

	TEST(VoxelDefinitionCollectionTest, AirIsImplicitAndNumericIDsAreValidated)
	{
		Collection::Builder builder;
		const Collection collection = builder.build();
		EXPECT_EQ(collection.size(), 1u);
		EXPECT_TRUE(collection.contains(0));
		EXPECT_TRUE(collection.at(0).isAir());
		EXPECT_FALSE(collection.contains(9));
		EXPECT_THROW((void)collection.at(9), spk::Exception);
		EXPECT_THROW(builder.add(0, createDefinition()), spk::Exception);
		EXPECT_THROW(builder.add(1, Definition::air()), spk::Exception);
		EXPECT_THROW(builder.add(Packed::MaxDefinitionId + 1, createDefinition()), spk::Exception);
		EXPECT_EQ(collection.size(), 1u);
	}

	TEST(VoxelDefinitionCollectionTest, RejectDuplicateIDWithoutOverwritingItsDefinition)
	{
		Collection::Builder builder;
		const Definition first = createDefinition();
		const Definition second = createDefinition();
		builder.add(17, first);
		EXPECT_THROW(builder.add(17, second), spk::Exception);
		const Collection collection = builder.build();
		EXPECT_EQ(collection.size(), 2u);
		EXPECT_TRUE(collection.at(17) == first);
		EXPECT_FALSE(collection.at(17) == second);
	}

	TEST(VoxelDefinitionCollectionTest, BracketLookupPreservesAirAndRejectsUnknownIDs)
	{
		const Definition stored = createDefinition();
		const Collection collection = makeCollection({{37, stored}});
		const Collection &readOnly = collection;
		EXPECT_TRUE(collection[37] == stored);
		EXPECT_TRUE(readOnly[37] == stored);
		EXPECT_TRUE(collection[0].isAir());
		EXPECT_EQ(collection.tryGet(0), &Definition::air());
		EXPECT_THROW((void)collection[38], spk::Exception);
		EXPECT_EQ(collection.size(), 2u);
	}

	TEST(VoxelDefinitionCollectionTest, HighestRepresentableSparseIDIsSupported)
	{
		const Definition edge = createDefinition();
		const Collection collection = makeCollection({{Packed::MaxDefinitionId, edge}});
		EXPECT_TRUE(collection.at(Packed::MaxDefinitionId) == edge);
		EXPECT_TRUE(collection.contains(Packed::MaxDefinitionId));
	}

	TEST(VoxelPackedFileTest, FileConstructorResolvesSparseIDsAndAllOrientations)
	{
		const Definition a = createDefinition("stone");
		const Definition b = createDefinition("glass");
		const Collection collection = makeCollection({{400, a}, {7, b}});
		const std::array<Cell::HorizontalOrientation, 4> horizontals{
			Cell::HorizontalOrientation::XPositive,
			Cell::HorizontalOrientation::ZNegative,
			Cell::HorizontalOrientation::XNegative,
			Cell::HorizontalOrientation::ZPositive};
		const std::array<Cell::VerticalOrientation, 2> verticals{
			Cell::VerticalOrientation::YPositive,
			Cell::VerticalOrientation::YNegative};
		std::vector<std::uint32_t> words;
		for (const auto vertical : verticals)
		{
			for (const auto horizontal : horizontals)
			{
				words.push_back(Packed::encode(vertical == verticals[0] ? 400 : 7, horizontal, vertical));
			}
		}
		PackedFile file;
		file.write(words);
		Volume::Builder builder({4, 2, 1});
		builder.bind(collection);
		builder.loadPacked(file.path);
		const Volume volume = builder.build();
		for (std::uint32_t index = 0; index < 8; ++index)
		{
			EXPECT_EQ(volume.atRaw({index % 4, index / 4, 0}), words[index]);
			const Cell cell = volume.at({index % 4, index / 4, 0});
			EXPECT_TRUE(cell.definition() == (index < 4 ? a : b));
			EXPECT_EQ(cell.horizontalOrientation(), horizontals[index % 4]);
			EXPECT_EQ(cell.verticalOrientation(), verticals[index / 4]);
		}
	}

	TEST(VoxelPackedFileTest, FileAirKeepsEncodedOrientationWithoutRegisteringDefinition)
	{
		PackedFile file;
		file.write({Packed::encode(0, Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YNegative)});
		Volume::Builder builder({1, 1, 1});
		builder.bind(Collection());
		builder.loadPacked(file.path);
		const Volume volume = builder.build();
		const Cell cell = volume.at({0, 0, 0});
		EXPECT_TRUE(cell.isAir());
		EXPECT_EQ(cell.horizontalOrientation(), Cell::HorizontalOrientation::ZPositive);
		EXPECT_EQ(cell.verticalOrientation(), Cell::VerticalOrientation::YNegative);
	}

	TEST(VoxelPackedFileTest, FileIsInterpretedAsLittleEndianAndXFastest)
	{
		const Definition first = createDefinition();
		const Definition second = createDefinition();
		const Collection collection = makeCollection({{0x010203, first}, {0x100012, second}});
		PackedFile file;
		std::vector<std::uint32_t> words;
		words.push_back(Packed::encode(0x010203, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive));
		words.push_back(Packed::encode(0x100012, Cell::HorizontalOrientation::XNegative, Cell::VerticalOrientation::YNegative));
		words.push_back(0);
		words.push_back(Packed::encode(0x010203, Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YPositive));
		file.write(words);
		Volume::Builder builder({2, 2, 1});
		builder.bind(collection);
		builder.loadPacked(file.path);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.at({1, 0, 0}).definition() == second);
		EXPECT_TRUE(volume.at({0, 1, 0}).isAir());
		EXPECT_TRUE(volume.at({1, 1, 0}).definition() == first);
	}

	TEST(VoxelPackedFileTest, AliasedIDsAreRetainedAcrossVolumeCopies)
	{
		const Definition first = createDefinition();
		const Collection collection = makeCollection({{7, first}, {20, first}});
		PackedFile file;
		file.write({Packed::encode(7, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive), Packed::encode(20, Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YNegative)});
		Volume::Builder builder({2, 1, 1});
		builder.bind(collection);
		builder.loadPacked(file.path);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.at({1, 0, 0}).definition() == first);
		Volume::Builder changed(volume.dimensions());
		changed.copyFrom(volume);
		changed.erase({0, 0, 0});
		EXPECT_TRUE(changed.build().at({1, 0, 0}).definition() == first);
	}

	TEST(VoxelPackedFileTest, BindUsesImmutableSnapshotsAndAllowsExplicitReplacement)
	{
		const Definition first = createDefinition();
		const Definition second = createDefinition();
		Collection::Builder definitions;
		definitions.add(4, first);
		const Collection before = definitions.build();
		Volume::Builder builder({2, 1, 1});
		EXPECT_THROW(builder.set({0, 0, 0}, 4), spk::Exception);
		builder.bind(before);
		definitions.add(5, second);
		const Collection after = definitions.build();
		EXPECT_THROW(builder.set({1, 0, 0}, 5), spk::Exception);
		builder.set({0, 0, 0}, 4, Cell::HorizontalOrientation::ZNegative);
		builder.bind(after);
		builder.set({1, 0, 0}, 5, Cell::HorizontalOrientation::XNegative, Cell::VerticalOrientation::YNegative);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.at({1, 0, 0}).definition() == second);
	}

	TEST(VoxelPackedFileTest, BoundCollectionOutlivesItsOriginalOwner)
	{
		Volume::Builder builder({1, 1, 1});
		const Definition definition = createDefinition();
		builder.bind(makeCollection({{3, definition}}));
		builder.set({0, 0, 0}, 3);
		EXPECT_TRUE(builder.build().at({0, 0, 0}).definition() == definition);
	}

	TEST(VoxelPackedFileTest, MissingFileAndInvalidSizesAreRejected)
	{
		PackedFile file;
		Volume::Builder builder({1, 1, 1});
		builder.bind(Collection());
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		file.write({});
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		file.write({0, 0});
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		file.write({0});
		EXPECT_NO_THROW(builder.loadPacked(file.path));
		EXPECT_TRUE(builder.build().at({0, 0, 0}).isAir());
	}

	TEST(VoxelPackedFileTest, UnknownIDsRejectLoadWithoutChangingPreviouslyEditedBuilder)
	{
		const Definition a = createDefinition();
		Collection::Builder definitions;
		definitions.add(2, a);
		Volume::Builder builder({2, 1, 1});
		builder.bind(definitions.build());
		builder.set({0, 0, 0}, 2);
		PackedFile file;
		file.write({Packed::encode(2, Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YPositive), Packed::encode(19, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive)});
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		const Volume unchanged = Volume::Builder(builder).build();
		EXPECT_TRUE(unchanged.at({0, 0, 0}).definition() == a);
		EXPECT_TRUE(unchanged.at({1, 0, 0}).isAir());
		definitions.add(19, createDefinition());
		builder.bind(definitions.build());
		builder.loadPacked(file.path);
		const Volume updated = builder.build();
		EXPECT_EQ(updated.at({0, 0, 0}).horizontalOrientation(), Cell::HorizontalOrientation::ZPositive);
		EXPECT_FALSE(updated.at({1, 0, 0}).isAir());
	}

	TEST(VoxelPackedFileTest, LoadingASecondFileReplacesCellsWithoutRemapping)
	{
		const Definition first = createDefinition();
		const Definition second = createDefinition();
		const Collection collection = makeCollection({{101, first}, {3, second}});
		Volume::Builder builder({2, 1, 1});
		builder.bind(collection);
		PackedFile file;
		file.write({Packed::encode(101, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive), Packed::encode(101, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive)});
		builder.loadPacked(file.path);
		file.write({Packed::encode(3, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive), 0});
		builder.loadPacked(file.path);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == second);
		EXPECT_TRUE(volume.at({1, 0, 0}).isAir());
		EXPECT_FALSE(volume.at({0, 0, 0}).definition() == first);
	}

	TEST(VoxelPackedFileTest, FailedLoadCanBeFixedAndRetriedWithoutRebinding)
	{
		const Definition first = createDefinition();
		const Collection collection = makeCollection({{1, first}});
		PackedFile file;
		file.write({0, Packed::encode(1, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive)});
		Volume::Builder builder({2, 1, 1});
		builder.bind(collection);
		builder.set({0, 0, 0}, 1);
		EXPECT_NO_THROW(builder.loadPacked(file.path));
		file.write({0});
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		file.write({Packed::encode(1, Cell::HorizontalOrientation::ZNegative, Cell::VerticalOrientation::YPositive), 0});
		EXPECT_NO_THROW(builder.loadPacked(file.path));
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == first);
		EXPECT_EQ(volume.at({0, 0, 0}).horizontalOrientation(), Cell::HorizontalOrientation::ZNegative);
		EXPECT_TRUE(volume.at({1, 0, 0}).isAir());
	}

	TEST(VoxelPackedFileTest, BindingOrLoadingAfterBuildIsRejected)
	{
		PackedFile file;
		file.write({0});
		Volume::Builder builder({1, 1, 1});
		builder.bind(Collection());
		const Volume volume = builder.build();
		EXPECT_THROW(builder.bind(Collection()), spk::Exception);
		EXPECT_THROW(builder.copyFrom(volume), spk::Exception);
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		EXPECT_THROW(builder.set({0, 0, 0}, 0), spk::Exception);
		EXPECT_TRUE(volume.at({0, 0, 0}).isAir());
	}
}
