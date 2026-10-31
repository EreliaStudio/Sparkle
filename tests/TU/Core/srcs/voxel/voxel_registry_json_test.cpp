#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "exception.hpp"
#include "voxel/definition.hpp"
#include "voxel/volume.hpp"

namespace
{
	using Shape = spk::Voxel::Shape;
	using Definition = spk::Voxel::Definition;
	using Volume = spk::Voxel::Volume;

	static_assert(std::is_default_constructible_v<Shape::Builder>);
	static_assert(!std::is_constructible_v<Shape::Builder, const spk::JSON::Value &>);
	static_assert(std::is_default_constructible_v<Definition::Registry::Builder>);
	static_assert(!std::is_constructible_v<Definition::Registry::Builder, const Shape::Registry &>);
	static_assert(std::is_default_constructible_v<Volume::Builder>);
	static_assert(!std::is_constructible_v<Volume::Builder, spk::Vector3UInt>);
	static_assert(!std::is_constructible_v<Volume::Builder, const Volume &>);
	static_assert(!std::is_constructible_v<Volume::Builder, spk::Vector3UInt, const std::filesystem::path &, const Definition::Registry &>);
	static_assert(std::is_default_constructible_v<Definition::Builder>);
	static_assert(!std::is_constructible_v<Definition::Builder, const Shape::Registry &>);
	static_assert(!std::is_constructible_v<Definition::Builder, const Shape::Registry &, const spk::JSON::Value &>);

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
		Shape::Registry::Builder builder;
		builder.load(JSONFile(Shapes).path());
		return builder.build();
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

	TEST(VoxelShapeRegistryTest, UnknownIDReportsRequestedString)
	{
		const Shape::Registry registry = shapeRegistry();
		EXPECT_FALSE(registry.contains("absent_shape"));
		EXPECT_EQ(registry.tryGet("absent_shape"), nullptr);
		try
		{
			(void)registry["absent_shape"];
			FAIL() << "Unknown Shape ID must throw";
		} catch (const spk::Exception &exception)
		{
			EXPECT_NE(std::string(exception.what()).find("absent_shape"), std::string::npos);
		}
	}

	TEST(VoxelShapeRegistryTest, SupportsProgrammaticAddAndRejectsDuplicateIDs)
	{
		Shape::Registry::Builder builder;
		Shape cube = Shape::Builder{}.build();
		builder.add("cube", cube);
		const Shape::Registry registry = builder.build();
		EXPECT_TRUE(registry["cube"] == cube);
		EXPECT_THROW(builder.add("cube", cube), spk::Exception);
		EXPECT_THROW(builder.add("", cube), spk::Exception);
		EXPECT_EQ(registry.size(), 1u);
	}

	TEST(VoxelShapeRegistryTest, SupportsIndividualJSONCatalogElement)
	{
		const JSONFile file(R"({"id":"single","data":{"slots":{},"polygons":[]}})");
		Shape::Registry::Builder builder;
		builder.load(file.path());
		const Shape::Registry registry = builder.build();
		EXPECT_TRUE(registry.contains("single"));
		EXPECT_TRUE(registry["single"].empty());
	}

	TEST(VoxelShapeRegistryTest, RejectsDuplicateJSONEntries)
	{
		const JSONFile file(R"({"elements":[
			{"id":"same","data":{"slots":{},"polygons":[]}},
			{"id":"same","data":{"slots":{},"polygons":[]}}
		]})");
		Shape::Registry::Builder builder;
		EXPECT_THROW(builder.load(file.path()), spk::Exception);
		EXPECT_EQ(builder.build().size(), 1u);
	}

	TEST(VoxelDefinitionBuilderTest, UsesRegisteredShapeAndBuildsImmutableDefinitions)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Builder builder;
		builder.bind(shapes);
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

	TEST(VoxelDefinitionBuilderTest, MissingShapeOrSlotsRejectBuild)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Builder builder;
		EXPECT_THROW((void)builder.build(), spk::Exception);
		EXPECT_THROW(builder.setShape("missing"), spk::Exception);
		builder.bind(shapes);
		EXPECT_THROW((void)builder.build(), spk::Exception);
		builder.setShape("cube");
		EXPECT_THROW((void)builder.build(), spk::Exception);
	}

	TEST(VoxelDefinitionBuilderTest, FailedJSONParseLeavesPreviousStateIntact)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Builder builder;
		builder.bind(shapes);
		builder.setShape("cube");
		builder.setSlots({{"face", "original"}});
		const auto invalid = spk::JSON::Value::fromString(
			R"({"shape":"absent","slots":{"face":"bad"}})");
		EXPECT_THROW(builder.addFromJSON(invalid), spk::Exception);
		EXPECT_EQ(builder.build().semanticSlot("face"), "original");
	}

	TEST(VoxelDefinitionBuilderTest, RegistryBindingOutlivesOriginalRegistry)
	{
		Definition::Builder builder;
		{
			const Shape::Registry shapes = shapeRegistry();
			builder.bind(shapes);
		}
		builder.setShape("slab");
		builder.setSlots({{"side", "transparent"}});
		const Definition definition = builder.build();
		EXPECT_FLOAT_EQ(definition.shape().slot("side").opacity, 0.75f);
	}

	TEST(VoxelDefinitionBuilderTest, EmptyRegistryRejectsUnknownIDsAndBindingEnablesJSON)
	{
		Definition::Builder builder;
		const auto json = spk::JSON::Value::fromString(
			R"({"shape":"cube","slots":{"face":"stone"}})");
		EXPECT_THROW(builder.setShape("cube"), spk::Exception);
		EXPECT_THROW(builder.addFromJSON(json), spk::Exception);
		EXPECT_THROW((void)builder.build(), spk::Exception);
		const Shape::Registry shapes = shapeRegistry();
		builder.bind(shapes);
		builder.addFromJSON(json);
		EXPECT_EQ(builder.build().semanticSlot("face"), "stone");
	}

	TEST(VoxelDefinitionBuilderTest, RebindingRequiresReselectingShape)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Builder builder;
		builder.bind(shapes);
		builder.setShape("cube");
		builder.setSlots({{"face", "stone"}});
		const Definition original = builder.build();
		builder.bind(Shape::Registry());
		EXPECT_THROW((void)builder.build(), spk::Exception);
		builder.setSlots({});
		EXPECT_THROW((void)builder.build(), spk::Exception);
		EXPECT_THROW(builder.setShape("cube"), spk::Exception);
		builder.bind(shapes);
		EXPECT_THROW((void)builder.build(), spk::Exception);
		builder.setShape("cube");
		builder.setSlots({{"face", "stone"}});
		EXPECT_TRUE(builder.build().shape() == original.shape());
	}

	TEST(VoxelDefinitionBuilderTest, ExplicitlySelectedEmptyShapeIsNotAir)
	{
		Shape::Registry::Builder shapeBuilder;
		shapeBuilder.add("empty", Shape::Builder{}.build());
		const Shape::Registry shapes = shapeBuilder.build();
		Definition::Builder builder;
		builder.bind(shapes);
		EXPECT_THROW((void)builder.build(), spk::Exception);
		builder.setShape("empty");
		const Definition result = builder.build();
		EXPECT_TRUE(result.shape().empty());
		EXPECT_TRUE(result.slots().empty());
		EXPECT_FALSE(result.isAir());
	}

	TEST(VoxelDefinitionRegistryTest, LoadsNumericIDsThroughJSONCatalog)
	{
		const Shape::Registry shapes = shapeRegistry();
		const JSONFile file(Definitions);
		Definition::Registry::Builder builder;
		builder.bind(shapes);
		builder.load(file.path());
		const Definition::Registry registry = builder.build();
		EXPECT_EQ(registry.size(), 3u);
		EXPECT_TRUE(registry[0].isAir());
		EXPECT_TRUE(registry[42].shape() == shapes["cube"]);
		EXPECT_EQ(registry[42].semanticSlot("face"), "stone");
		EXPECT_EQ(registry[7].semanticSlot("side"), "glass");
		EXPECT_THROW((void)registry[10], spk::Exception);
	}

	TEST(VoxelDefinitionRegistryTest, AirAccessIsConsistentAcrossEveryLookupMethod)
	{
		const Definition::Registry registry;
		const Definition *air = &Definition::air();
		EXPECT_TRUE(registry.contains(0));
		EXPECT_EQ(registry.tryGet(0), air);
		EXPECT_EQ(&registry.at(0), air);
		EXPECT_EQ(&registry[0], air);
		EXPECT_EQ(registry.size(), 1u);
	}

	TEST(VoxelDefinitionRegistryTest, UnknownAndRegisteredIDsMatchLookupContracts)
	{
		const Shape::Registry shapes = shapeRegistry();
		const JSONFile file(Definitions);
		Definition::Registry::Builder builder;
		builder.bind(shapes);
		builder.load(file.path());
		const Definition::Registry registry = builder.build();
		EXPECT_EQ(registry.tryGet(42), &registry.at(42));
		EXPECT_EQ(registry.tryGet(7), &registry[7]);
		EXPECT_FALSE(registry.contains(999));
		EXPECT_EQ(registry.tryGet(999), nullptr);
		try
		{
			(void)registry.at(999);
			FAIL() << "Unknown Definition ID must throw";
		} catch (const spk::Exception &exception)
		{
			EXPECT_NE(std::string(exception.what()).find("999"), std::string::npos);
		}
		EXPECT_THROW((void)registry[999], spk::Exception);
	}

	TEST(VoxelDefinitionRegistryTest, JSONRejectsUnboundShapesAndUnknownShapeKeys)
	{
		const JSONFile file(Definitions);
		Definition::Registry::Builder unbound;
		EXPECT_THROW(unbound.load(file.path()), spk::Exception);
		Shape::Registry shapes;
		Definition::Registry::Builder builder;
		builder.bind(shapes);
		EXPECT_THROW(builder.load(file.path()), spk::Exception);
	}

	TEST(VoxelDefinitionRegistryTest, JSONRejectsReservedAndOversizedIDs)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Registry::Builder builder;
		builder.bind(shapes);
		const JSONFile air(R"({"id":0,"data":{"shape":"cube","slots":{"face":"stone"}}})");
		const JSONFile oversized(R"({"id":536870912,"data":{"shape":"cube","slots":{"face":"stone"}}})");
		EXPECT_THROW(builder.load(air.path()), spk::Exception);
		EXPECT_THROW(builder.load(oversized.path()), spk::Exception);
		EXPECT_EQ(builder.build().size(), 1u);
	}

	TEST(VoxelDefinitionRegistryTest, ProgrammaticAddChecksBoundsAndAir)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Builder builder;
		builder.bind(shapes);
		builder.setShape("cube");
		builder.setSlots({{"face", "stone"}});
		const Definition stone = builder.build();
		Definition::Registry::Builder builderRegistry;
		builderRegistry.add(13, stone);
		const Definition::Registry registry = builderRegistry.build();
		EXPECT_TRUE(registry[13] == stone);
		EXPECT_THROW(builderRegistry.add(13, stone), spk::Exception);
		EXPECT_THROW(builderRegistry.add(0, stone), spk::Exception);
		EXPECT_THROW(builderRegistry.add(14, Definition::air()), spk::Exception);
		EXPECT_EQ(registry.size(), 2u);
	}

	TEST(VoxelShapeRegistryTest, PublishedSnapshotsShareDataButIgnoreLaterBuilderChanges)
	{
		Shape::Registry::Builder builder;
		const Shape cube = Shape::Builder{}.build();
		builder.add("cube", cube);
		const Shape::Registry first = builder.build();
		const Shape::Registry copy = first;
		EXPECT_EQ(&first["cube"], &copy["cube"]);
		builder.add("slab", Shape::Builder{}.build());
		const Shape::Registry second = builder.build();
		EXPECT_FALSE(first.contains("slab"));
		EXPECT_TRUE(second.contains("slab"));
		EXPECT_TRUE(first["cube"] == second["cube"]);
	}

	TEST(VoxelDefinitionRegistryTest, PublishedSnapshotsShareDataButIgnoreLaterBuilderChanges)
	{
		const Shape::Registry shapes = shapeRegistry();
		Definition::Builder definitionBuilder;
		definitionBuilder.bind(shapes);
		definitionBuilder.setShape("cube");
		definitionBuilder.setSlots({{"face", "rock"}});
		Definition::Registry::Builder builder;
		builder.add(9, definitionBuilder.build());
		const Definition::Registry first = builder.build();
		const Definition::Registry copy = first;
		EXPECT_EQ(&first[9], &copy[9]);
		definitionBuilder.setSlots({{"face", "glass"}});
		builder.add(20, definitionBuilder.build());
		const Definition::Registry second = builder.build();
		EXPECT_FALSE(first.contains(20));
		EXPECT_TRUE(second.contains(20));
		EXPECT_TRUE(first[9] == second[9]);
	}

	TEST(VoxelDefinitionRegistryTest, LoadedDefinitionsBindToVolumeByNumericID)
	{
		const Shape::Registry shapes = shapeRegistry();
		const JSONFile file(Definitions);
		Definition::Registry::Builder registryBuilder;
		registryBuilder.bind(shapes);
		registryBuilder.load(file.path());
		const Definition::Registry registry = registryBuilder.build();
		Volume::Builder builder;
		builder.setDimensions({2, 1, 1});
		builder.bind(registry);
		builder.set({1, 0, 0}, 42);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).isAir());
		EXPECT_TRUE(volume.at({1, 0, 0}).definition() == registry[42]);
	}
}
