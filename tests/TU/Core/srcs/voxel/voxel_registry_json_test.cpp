#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "exception.hpp"
#include "voxel/definition.hpp"
#include "voxel/volume.hpp"

namespace
{
	using Shape = spk::Voxel::Shape;
	using Definition = spk::Voxel::Definition;
	using Volume = spk::Voxel::Volume;

	class JSONFile
	{
	private:
		std::filesystem::path _path;

	public:
		explicit JSONFile(const std::string &content)
		{
			static std::atomic<std::uint64_t> next{0};
			const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
			_path = std::filesystem::temp_directory_path() /
					("sparkle-voxel-catalog-" + std::to_string(ticks) + "-" + std::to_string(++next) + ".json");
			std::ofstream output(_path, std::ios::binary);
			output << content;
			if (!output)
			{
				throw std::runtime_error("Unable to write Voxel JSON fixture");
			}
		}

		~JSONFile()
		{
			std::error_code error;
			std::filesystem::remove(_path, error);
		}

		[[nodiscard]] const std::filesystem::path &path() const noexcept
		{
			return _path;
		}
	};

	constexpr auto Shapes = R"({"elements":[
		{"id":"cube","data":{"slots":{"face":1},"polygons":[]}},
		{"id":"slab","data":{"slots":{"side":0.75},"polygons":[]}}
	]})";
	constexpr auto Definitions = R"({"elements":[
		{"id":42,"data":{"shape":"cube","slots":{"face":"stone"}}},
		{"id":7,"data":{"shape":"slab","slots":{"side":"glass"}}}
	]})";

	Shape::Registry shapeRegistry()
	{
		Shape::Registry result;
		result.load(JSONFile(Shapes).path());
		return result;
	}

	TEST(VoxelShapeRegistryTest, LoadsNamedShapesThroughJSONCatalog)
	{
		const Shape::Registry registry = shapeRegistry();
		EXPECT_EQ(registry.size(), 2u);
		EXPECT_TRUE(registry.contains("cube"));
		EXPECT_TRUE(registry.contains("slab"));
		EXPECT_FLOAT_EQ(registry["slab"].slot("side").opacity, 0.75f);
		EXPECT_EQ(&registry.at("cube"), &registry["cube"]);
		EXPECT_THROW((void)registry["missing"], spk::Exception);
	}

	TEST(VoxelShapeRegistryTest, SupportsProgrammaticAddAndRejectsDuplicateIDs)
	{
		Shape::Registry registry;
		Shape cube = Shape::Builder{}.build();
		registry.add("cube", cube);
		EXPECT_TRUE(registry["cube"] == cube);
		EXPECT_THROW(registry.add("cube", cube), spk::Exception);
		EXPECT_THROW(registry.add("", cube), spk::Exception);
		EXPECT_EQ(registry.size(), 1u);
	}

	TEST(VoxelShapeRegistryTest, SupportsIndividualJSONCatalogElement)
	{
		const JSONFile file(R"({"id":"single","data":{"slots":{},"polygons":[]}})");
		Shape::Registry registry;
		registry.load(file.path());
		EXPECT_TRUE(registry.contains("single"));
		EXPECT_TRUE(registry["single"].empty());
	}

	TEST(VoxelShapeRegistryTest, RejectsDuplicateJSONEntries)
	{
		const JSONFile file(R"({"elements":[
			{"id":"same","data":{"slots":{},"polygons":[]}},
			{"id":"same","data":{"slots":{},"polygons":[]}}
		]})");
		Shape::Registry registry;
		EXPECT_THROW(registry.load(file.path()), spk::Exception);
		EXPECT_EQ(registry.size(), 1u);
	}

	TEST(VoxelDefinitionBuilderTest, UsesRegisteredShapeAndBuildsImmutableDefinitions)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Builder builder(shapes);
		builder.setShape("cube");
		builder.setSlots({{"face", "rock"}});
		const Definition first = builder.build();
		builder.setSlots({{"face", "glass"}});
		const Definition second = builder.build();
		EXPECT_TRUE(first.shape() == shapes["cube"]);
		EXPECT_TRUE(second.shape() == shapes["cube"]);
		EXPECT_FALSE(first == second);
		EXPECT_EQ(first.semanticSlot("face"), "rock");
		EXPECT_EQ(second.semanticSlot("face"), "glass");
	}

	TEST(VoxelDefinitionBuilderTest, CannotBuildWithoutShapeOrWithMissingSlots)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Builder builder(shapes);
		EXPECT_THROW((void)builder.build(), spk::Exception);
		EXPECT_THROW(builder.setShape("missing"), spk::Exception);
		builder.setShape("cube");
		EXPECT_THROW((void)builder.build(), spk::Exception);
	}

	TEST(VoxelDefinitionBuilderTest, FailedJSONParseLeavesPreviousStateIntact)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Builder builder(shapes);
		builder.setShape("cube");
		builder.setSlots({{"face", "original"}});
		const auto invalid = spk::JSON::Value::fromString(
			R"({"shape":"absent","slots":{"face":"bad"}})");
		EXPECT_THROW(builder.addFromJSON(invalid), spk::Exception);
		EXPECT_EQ(builder.build().semanticSlot("face"), "original");
	}

	TEST(VoxelDefinitionBuilderTest, RegistryBindingOutlivesOriginalRegistry)
	{
		Definition::Builder builder(shapeRegistry());
		builder.setShape("slab");
		builder.setSlots({{"side", "transparent"}});
		const Definition definition = builder.build();
		EXPECT_FLOAT_EQ(definition.shape().slot("side").opacity, 0.75f);
	}

	TEST(VoxelDefinitionRegistryTest, LoadsNumericIDsThroughJSONCatalog)
	{
		const Shape::Registry shapes = shapeRegistry();
		const JSONFile file(Definitions);
		Definition::Registry registry(shapes);
		registry.load(file.path());
		EXPECT_EQ(registry.size(), 3u);
		EXPECT_TRUE(registry[0].isAir());
		EXPECT_TRUE(registry[42].shape() == shapes["cube"]);
		EXPECT_EQ(registry[42].semanticSlot("face"), "stone");
		EXPECT_EQ(registry[7].semanticSlot("side"), "glass");
		EXPECT_THROW((void)registry[10], spk::Exception);
	}

	TEST(VoxelDefinitionRegistryTest, JSONRejectsUnboundShapesAndUnknownShapeKeys)
	{
		const JSONFile file(Definitions);
		Definition::Registry unbound;
		EXPECT_THROW(unbound.load(file.path()), spk::Exception);
		Shape::Registry shapes;
		Definition::Registry registry(shapes);
		EXPECT_THROW(registry.load(file.path()), spk::Exception);
	}

	TEST(VoxelDefinitionRegistryTest, JSONRejectsReservedAndOversizedIDs)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Registry registry(shapes);
		const JSONFile air(R"({"id":0,"data":{"shape":"cube","slots":{"face":"stone"}}})");
		const JSONFile oversized(R"({"id":536870912,"data":{"shape":"cube","slots":{"face":"stone"}}})");
		EXPECT_THROW(registry.load(air.path()), spk::Exception);
		EXPECT_THROW(registry.load(oversized.path()), spk::Exception);
		EXPECT_EQ(registry.size(), 1u);
	}

	TEST(VoxelDefinitionRegistryTest, ProgrammaticAddChecksBoundsAndAir)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Builder builder(shapes);
		builder.setShape("cube");
		builder.setSlots({{"face", "stone"}});
		const Definition stone = builder.build();
		Definition::Registry registry;
		registry.add(13, stone);
		EXPECT_TRUE(registry[13] == stone);
		EXPECT_THROW(registry.add(13, stone), spk::Exception);
		EXPECT_THROW(registry.add(0, stone), spk::Exception);
		EXPECT_THROW(registry.add(14, Definition::air()), spk::Exception);
		EXPECT_EQ(registry.size(), 2u);
	}

	TEST(VoxelDefinitionRegistryTest, LoadedDefinitionsBindToVolumeByNumericID)
	{
		const Shape::Registry shapes = shapeRegistry();
		const JSONFile file(Definitions);
		Definition::Registry registry(shapes);
		registry.load(file.path());
		Volume::Builder builder({2, 1, 1});
		builder.bind(registry);
		builder.set({1, 0, 0}, 42);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.cell({0, 0, 0}).isAir());
		EXPECT_TRUE(volume.cell({1, 0, 0}).definition() == registry[42]);
	}
}
