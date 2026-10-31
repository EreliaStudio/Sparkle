#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "voxel/definition_registry.hpp"
#include "voxel/detail/packed_cell.hpp"
#include "voxel/volume.hpp"

namespace
{
	using Volume = spk::Voxel::Volume;
	using Cell = spk::Voxel::Cell;
	using Shape = spk::Voxel::Shape;
	using Definition = spk::Voxel::Definition;
	using Registry = spk::Voxel::DefinitionRegistry;
	using Packed = spk::Voxel::Detail::PackedCell;
	using Position = spk::Vector3UInt;

	Definition createDefinition(std::string semantic = "stone")
	{
		Shape::Builder builder;
		builder.addSlot("face");
		return Definition(builder.build(), {{"face", std::move(semantic)}});
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
		Registry registry;
		EXPECT_EQ(registry.size(), 1u);
		EXPECT_TRUE(registry.contains(0));
		EXPECT_TRUE(registry.at(0).isAir());
		EXPECT_FALSE(registry.contains(9));
		EXPECT_THROW((void)registry.at(9), spk::Exception);
		EXPECT_THROW(registry.add(0, createDefinition()), spk::Exception);
		EXPECT_THROW(registry.add(1, Definition::air()), spk::Exception);
		EXPECT_THROW(registry.add(Packed::MaxDefinitionIndex + 1, createDefinition()), spk::Exception);
		EXPECT_EQ(registry.size(), 1u);
	}

	TEST(VoxelDefinitionRegistryTest, RejectDuplicateIDWithoutOverwritingItsDefinition)
	{
		Registry registry;
		const Definition first = createDefinition();
		const Definition second = createDefinition();
		registry.add(17, first);
		EXPECT_THROW(registry.add(17, second), spk::Exception);
		EXPECT_EQ(registry.size(), 2u);
		EXPECT_TRUE(registry.at(17) == first);
		EXPECT_FALSE(registry.at(17) == second);
	}

	TEST(VoxelDefinitionRegistryTest, HighestRepresentableSparseIDIsSupported)
	{
		Registry registry;
		const Definition edge = createDefinition();
		registry.add(Packed::MaxDefinitionIndex, edge);
		EXPECT_TRUE(registry.at(Packed::MaxDefinitionIndex) == edge);
		EXPECT_TRUE(registry.contains(Packed::MaxDefinitionIndex));
	}

	TEST(VoxelPackedFileTest, FileConstructorResolvesSparseIDsAndAllOrientations)
	{
		const Definition a = createDefinition("stone");
		const Definition b = createDefinition("glass");
		Registry registry;
		registry.add(400, a);
		registry.add(7, b);
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
		const Volume volume = Volume::Builder({4, 2, 1}, file.path, registry).build();
		for (std::uint32_t index = 0; index < 8; ++index)
		{
			const Cell cell = volume.cell({index % 4, index / 4, 0});
			EXPECT_TRUE(cell.definition() == (index < 4 ? a : b));
			EXPECT_EQ(cell.horizontalOrientation(), horizontals[index % 4]);
			EXPECT_EQ(cell.verticalOrientation(), verticals[index / 4]);
		}
	}

	TEST(VoxelPackedFileTest, FileAirKeepsEncodedOrientationWithoutRegisteringDefinition)
	{
		Registry registry;
		PackedFile file;
		file.write({Packed::encode(0, Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YNegative)});
		const Volume volume = Volume::Builder({1, 1, 1}, file.path, registry).build();
		const Cell cell = volume.cell({0, 0, 0});
		EXPECT_TRUE(cell.isAir());
		EXPECT_EQ(cell.horizontalOrientation(), Cell::HorizontalOrientation::ZPositive);
		EXPECT_EQ(cell.verticalOrientation(), Cell::VerticalOrientation::YNegative);
	}

	TEST(VoxelPackedFileTest, FileIsInterpretedAsLittleEndianAndXFastest)
	{
		const Definition first = createDefinition();
		const Definition second = createDefinition();
		Registry registry;
		registry.add(0x010203, first);
		registry.add(0x100012, second);
		PackedFile file;
		std::vector<std::uint32_t> words;
		words.push_back(Packed::encode(0x010203, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive));
		words.push_back(Packed::encode(0x100012, Cell::HorizontalOrientation::XNegative, Cell::VerticalOrientation::YNegative));
		words.push_back(0);
		words.push_back(Packed::encode(0x010203, Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YPositive));
		file.write(words);
		const Volume volume = Volume::Builder({2, 2, 1}, file.path, registry).build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.cell({1, 0, 0}).definition() == second);
		EXPECT_TRUE(volume.cell({0, 1, 0}).isAir());
		EXPECT_TRUE(volume.cell({1, 1, 0}).definition() == first);
	}

	TEST(VoxelPackedFileTest, AliasedIDsAndCopiesStillResolveByDefinitionIdentity)
	{
		const Definition first = createDefinition();
		Registry registry;
		registry.add(7, first);
		registry.add(20, first);
		PackedFile file;
		file.write({Packed::encode(7, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive), Packed::encode(20, Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YNegative)});
		Volume::Builder builder({2, 1, 1}, file.path, registry);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.cell({1, 0, 0}).definition() == first);
		Volume::Builder copied(volume);
		copied.erase({0, 0, 0});
		EXPECT_TRUE(copied.build().cell({1, 0, 0}).definition() == first);
	}

	TEST(VoxelPackedFileTest, BindAllowsSetByIDAndUsesAnOwnedRegistrySnapshot)
	{
		const Definition first = createDefinition();
		const Definition second = createDefinition();
		Volume::Builder builder({2, 1, 1});
		EXPECT_THROW(builder.set({0, 0, 0}, 4), spk::Exception);
		Registry registry;
		registry.add(4, first);
		builder.bind(registry);
		registry.add(5, second);
		EXPECT_THROW(builder.set({1, 0, 0}, 5), spk::Exception);
		EXPECT_THROW(builder.set({0, 0, 0}, 99), spk::Exception);
		builder.set({0, 0, 0}, 4, Cell::HorizontalOrientation::ZNegative);
		builder.bind(registry);
		builder.set({1, 0, 0}, 5, Cell::HorizontalOrientation::XNegative, Cell::VerticalOrientation::YNegative);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == first);
		EXPECT_TRUE(volume.cell({1, 0, 0}).definition() == second);
		EXPECT_EQ(volume.cell({1, 0, 0}).verticalOrientation(), Cell::VerticalOrientation::YNegative);
	}

	TEST(VoxelPackedFileTest, BoundRegistryOutlivesItsOriginalOwner)
	{
		Volume::Builder builder({1, 1, 1});
		Definition definition = createDefinition();
		{
			Registry registry;
			registry.add(3, definition);
			builder.bind(registry);
		}
		builder.set({0, 0, 0}, 3);
		EXPECT_TRUE(builder.build().cell({0, 0, 0}).definition() == definition);
	}

	TEST(VoxelPackedFileTest, MissingFileAndInvalidSizesAreRejected)
	{
		Registry registry;
		PackedFile file;
		EXPECT_THROW((void)Volume::Builder(Position(1, 1, 1), file.path, registry), spk::Exception);
		file.write({});
		EXPECT_THROW((void)Volume::Builder(Position(1, 1, 1), file.path, registry), spk::Exception);
		file.write({0, 0});
		EXPECT_THROW((void)Volume::Builder(Position(1, 1, 1), file.path, registry), spk::Exception);
		file.write({0});
		EXPECT_NO_THROW((void)Volume::Builder(Position(1, 1, 1), file.path, registry));
	}

	TEST(VoxelPackedFileTest, UnknownIDsRejectLoadWithoutChangingPreviouslyEditedBuilder)
	{
		const Definition a = createDefinition();
		Registry registry;
		registry.add(2, a);
		Volume::Builder builder({2, 1, 1});
		builder.bind(registry);
		builder.set({0, 0, 0}, Cell(a));
		PackedFile file;
		file.write({Packed::encode(2, Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YPositive), Packed::encode(19, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive)});
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		const Volume unchanged = Volume::Builder(builder).build();
		EXPECT_TRUE(unchanged.cell({0, 0, 0}).definition() == a);
		EXPECT_TRUE(unchanged.cell({1, 0, 0}).isAir());
		registry.add(19, createDefinition());
		builder.bind(registry);
		builder.loadPacked(file.path);
		const Volume updated = builder.build();
		EXPECT_EQ(updated.cell({0, 0, 0}).horizontalOrientation(), Cell::HorizontalOrientation::ZPositive);
		EXPECT_FALSE(updated.cell({1, 0, 0}).isAir());
	}

	TEST(VoxelPackedFileTest, LoadingASecondFileReplacesLocalDefinitionsTransactionally)
	{
		const Definition first = createDefinition();
		const Definition second = createDefinition();
		Registry registry;
		registry.add(101, first);
		registry.add(3, second);
		Volume::Builder builder({2, 1, 1});
		builder.bind(registry);
		PackedFile file;
		file.write({Packed::encode(101, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive), Packed::encode(101, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive)});
		builder.loadPacked(file.path);
		file.write({Packed::encode(3, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive), 0});
		builder.loadPacked(file.path);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == second);
		EXPECT_TRUE(volume.cell({1, 0, 0}).isAir());
		EXPECT_FALSE(volume.cell({0, 0, 0}).definition() == first);
	}

	TEST(VoxelPackedFileTest, FailedLoadCanBeFixedAndRetriedWithoutRebinding)
	{
		const Definition first = createDefinition();
		Registry registry;
		registry.add(1, first);
		PackedFile file;
		file.write({0, Packed::encode(1, Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive)});
		Volume::Builder builder({2, 1, 1});
		builder.bind(registry);
		builder.set({0, 0, 0}, Cell(first));
		EXPECT_NO_THROW(builder.loadPacked(file.path));
		file.write({0});
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		file.write({Packed::encode(1, Cell::HorizontalOrientation::ZNegative, Cell::VerticalOrientation::YPositive), 0});
		EXPECT_NO_THROW(builder.loadPacked(file.path));
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).definition() == first);
		EXPECT_EQ(volume.cell({0, 0, 0}).horizontalOrientation(), Cell::HorizontalOrientation::ZNegative);
		EXPECT_TRUE(volume.cell({1, 0, 0}).isAir());
	}

	TEST(VoxelPackedFileTest, BindingOrLoadingAfterBuildIsRejected)
	{
		Registry registry;
		PackedFile file;
		file.write({0});
		Volume::Builder builder({1, 1, 1});
		builder.bind(registry);
		const Volume volume = builder.build();
		EXPECT_THROW(builder.bind(registry), spk::Exception);
		EXPECT_THROW(builder.loadPacked(file.path), spk::Exception);
		EXPECT_THROW(builder.set({0, 0, 0}, 0), spk::Exception);
		EXPECT_TRUE(volume.cell({0, 0, 0}).isAir());
	}
}
