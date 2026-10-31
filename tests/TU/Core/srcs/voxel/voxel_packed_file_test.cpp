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
	using Registry = Definition::Registry;
	using Packed = Volume::PackedCell;
	using Position = spk::Vector3UInt;

	Definition createDefinition(std::string semantic = "stone")
	{
		Shape::Builder builder;
		builder.addSlot("face");
		return Definition(builder.build(), {{"face", std::move(semantic)}});
	}

	Registry makeRegistry(std::initializer_list<std::pair<std::uint32_t, Definition>> entries)
	{
		Registry::Builder builder;
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

	TEST(VoxelDefinitionRegistryTest, AirIsImplicitAndNumericIDsAreValidated)
	{
		Registry::Builder builder;
		const Registry registry = builder.build();
		EXPECT_EQ(registry.size(), 1u);
		EXPECT_TRUE(registry.contains(0));
		EXPECT_TRUE(registry.at(0).isAir());
		EXPECT_FALSE(registry.contains(9));
		EXPECT_THROW((void)registry.at(9), spk::Exception);
		EXPECT_THROW(builder.add(0, createDefinition()), spk::Exception);
		EXPECT_THROW(builder.add(1, Definition::air()), spk::Exception);
		EXPECT_THROW(builder.add(Packed::MaxDefinitionId + 1, createDefinition()), spk::Exception);
		EXPECT_EQ(registry.size(), 1u);
	}

	TEST(VoxelDefinitionRegistryTest, RejectDuplicateIDWithoutOverwritingItsDefinition)
	{
		Registry::Builder builder;
		const Definition first = createDefinition();
		const Definition second = createDefinition();
		builder.add(17, first);
		EXPECT_THROW(builder.add(17, second), spk::Exception);
		const Registry registry = builder.build();
		EXPECT_EQ(registry.size(), 2u);
		EXPECT_TRUE(registry.at(17) == first);
		EXPECT_FALSE(registry.at(17) == second);
	}

	TEST(VoxelDefinitionRegistryTest, BracketLookupPreservesAirAndRejectsUnknownIDs)
	{
		const Definition stored = createDefinition();
		const Registry registry = makeRegistry({{37, stored}});
		const Registry &readOnly = registry;
		EXPECT_TRUE(registry[37] == stored);
		EXPECT_TRUE(readOnly[37] == stored);
		EXPECT_TRUE(registry[0].isAir());
		EXPECT_EQ(registry.tryGet(0), &Definition::air());
		EXPECT_THROW((void)registry[38], spk::Exception);
		EXPECT_EQ(registry.size(), 2u);
	}

	TEST(VoxelDefinitionRegistryTest, HighestRepresentableSparseIDIsSupported)
	{
		const Definition edge = createDefinition();
		const Registry registry = makeRegistry({{Packed::MaxDefinitionId, edge}});
		EXPECT_TRUE(registry.at(Packed::MaxDefinitionId) == edge);
		EXPECT_TRUE(registry.contains(Packed::MaxDefinitionId));
	}

	TEST(VoxelPackedFileTest, FileConstructorResolvesSparseIDsAndAllOrientations)
	{
		const Definition a = createDefinition("stone");
		const Definition b = createDefinition("glass");
		const Registry registry = makeRegistry({{400, a}, {7, b}});
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
		Volume::Builder builder;
		builder.setDimensions({4, 2, 1});
		builder.bind(registry);
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
		Volume::Builder builder;
		builder.setDimensions({1, 1, 1});
		builder.bind(Registry());
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
		const Registry registry = makeRegistry({{0x010203, first}, {0x100012, second}});
		PackedFile file;
		std::vector<std::uint32_t> words;
		words.push_back(Packed::encode(0x010203, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive));
		words.push_back(Packed::encode(0x100012, Cell::HorizontalOrientation::XNegative, Cell::VerticalOrientation::YNegative));
		words.push_back(0);
		words.push_back(Packed::encode(0x010203, Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YPositive));
		file.write(words);
		Volume::Builder builder;
		builder.setDimensions({2, 2, 1});
		builder.bind(registry);
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
		const Registry registry = makeRegistry({{7, first}, {20, first}});
		PackedFile file;
		file.write({Packed::encode(7, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive), Packed::encode(20, Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YNegative)});
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		builder.bind(registry);
		builder.loadPacked(file.path);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.at({1, 0, 0}).definition() == first);
		Volume::Builder changed;
		changed.load(volume);
		changed.erase({0, 0, 0});
		EXPECT_TRUE(changed.build().at({1, 0, 0}).definition() == first);
	}

	TEST(VoxelPackedFileTest, BindUsesImmutableSnapshotsAndAllowsExplicitReplacement)
	{
		const Definition first = createDefinition();
		const Definition second = createDefinition();
		Registry::Builder definitions;
		definitions.add(4, first);
		const Registry before = definitions.build();
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		EXPECT_THROW(builder.set({0, 0, 0}, 4), spk::Exception);
		builder.bind(before);
		definitions.add(5, second);
		const Registry after = definitions.build();
		EXPECT_THROW(builder.set({1, 0, 0}, 5), spk::Exception);
		builder.set({0, 0, 0}, 4, Cell::HorizontalOrientation::ZNegative);
		builder.bind(after);
		builder.set({1, 0, 0}, 5, Cell::HorizontalOrientation::XNegative, Cell::VerticalOrientation::YNegative);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.at({1, 0, 0}).definition() == second);
	}

	TEST(VoxelPackedFileTest, BoundRegistryOutlivesItsOriginalOwner)
	{
		Volume::Builder builder;
		builder.setDimensions({1, 1, 1});
		const Definition definition = createDefinition();
		builder.bind(makeRegistry({{3, definition}}));
		builder.set({0, 0, 0}, 3);
		EXPECT_TRUE(builder.build().at({0, 0, 0}).definition() == definition);
	}

	TEST(VoxelPackedFileTest, MissingFileAndInvalidSizesAreRejected)
	{
		PackedFile file;
		Volume::Builder builder;
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		builder.setDimensions({1, 1, 1});
		builder.bind(Registry());
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
		Registry::Builder definitions;
		definitions.add(2, a);
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
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
		const Registry registry = makeRegistry({{101, first}, {3, second}});
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		builder.bind(registry);
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
		const Registry registry = makeRegistry({{1, first}});
		PackedFile file;
		file.write({0, Packed::encode(1, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive)});
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		builder.bind(registry);
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
		Volume::Builder builder;
		builder.setDimensions({1, 1, 1});
		builder.bind(Registry());
		const Volume volume = builder.build();
		EXPECT_THROW(builder.bind(Registry()), spk::Exception);
		EXPECT_THROW(builder.setDimensions({1, 1, 1}), spk::Exception);
		EXPECT_THROW(builder.load(volume), spk::Exception);
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		EXPECT_THROW(builder.set({0, 0, 0}, 0), spk::Exception);
		EXPECT_TRUE(volume.at({0, 0, 0}).isAir());
	}
}
