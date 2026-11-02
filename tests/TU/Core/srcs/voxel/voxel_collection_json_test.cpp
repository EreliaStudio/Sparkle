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
	static_assert(std::is_default_constructible_v<Definition::Collection::Builder>);
	static_assert(!std::is_constructible_v<Definition::Collection::Builder, const Shape::Collection &>);
	static_assert(!std::is_default_constructible_v<Volume::Builder>);
	static_assert(std::is_constructible_v<Volume::Builder, spk::Vector3UInt>);
	static_assert(!std::is_constructible_v<Volume::Builder, const Volume &>);
	static_assert(!std::is_constructible_v<Volume::Builder, spk::Vector3UInt, const std::filesystem::path &, const Definition::Collection &>);
	static_assert(std::is_default_constructible_v<Definition::Builder>);
	static_assert(!std::is_constructible_v<Definition::Builder, const Shape::Collection &>);
	static_assert(!std::is_constructible_v<Definition::Builder, const Shape::Collection &, const spk::JSON::Value &>);

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
		{"id":"cube","data":{"slots":{"face":null},"polygons":[]}},
		{"id":"slab","data":{"slots":{"side":null},"polygons":[]}}
	]})";
	constexpr auto Definitions = R"({"elements":[
		{"id":42,"data":{"shape":"cube","slots":{"face":"stone"}}},
		{"id":7,"data":{"shape":"slab","slots":{"side":{"name":"glass","opacity":0.75}}}}
	]})";

	Shape::Collection shapeCollection()
	{
		Shape::Collection::Builder builder;
		builder.load(JSONFile(Shapes).path());
		return builder.build();
	}

	TEST(VoxelShapeCollectionTest, LoadsNamedShapesThroughJSONCatalog)
	{
		const Shape::Collection collection = shapeCollection();
		EXPECT_EQ(collection.size(), 2u);
		EXPECT_TRUE(collection.contains("cube"));
		EXPECT_TRUE(collection.contains("slab"));
		EXPECT_EQ(collection["slab"].slot("side").name, "side");
		EXPECT_EQ(&collection.at("cube"), &collection["cube"]);
		EXPECT_THROW((void)collection["missing"], spk::Exception);
	}

	TEST(VoxelShapeCollectionTest, UnknownIDReportsRequestedString)
	{
		const Shape::Collection collection = shapeCollection();
		EXPECT_FALSE(collection.contains("absent_shape"));
		EXPECT_EQ(collection.tryGet("absent_shape"), nullptr);
		try
		{
			(void)collection["absent_shape"];
			FAIL() << "Unknown Shape ID must throw";
		} catch (const spk::Exception &exception)
		{
			EXPECT_NE(std::string(exception.what()).find("absent_shape"), std::string::npos);
		}
	}

	TEST(VoxelShapeCollectionTest, SupportsProgrammaticAddAndRejectsDuplicateIDs)
	{
		Shape::Collection::Builder builder;
		Shape cube = Shape::Builder{}.build();
		builder.add("cube", cube);
		const Shape::Collection collection = builder.build();
		EXPECT_TRUE(collection["cube"] == cube);
		EXPECT_THROW(builder.add("cube", cube), spk::Exception);
		EXPECT_THROW(builder.add("", cube), spk::Exception);
		EXPECT_EQ(collection.size(), 1u);
	}

	TEST(VoxelShapeCollectionTest, SupportsIndividualJSONCatalogElement)
	{
		const JSONFile file(R"({"id":"single","data":{"slots":{},"polygons":[]}})");
		Shape::Collection::Builder builder;
		builder.load(file.path());
		const Shape::Collection collection = builder.build();
		EXPECT_TRUE(collection.contains("single"));
		EXPECT_TRUE(collection["single"].empty());
	}

	TEST(VoxelShapeCollectionTest, EmptyJSONIDReportsMemberPath)
	{
		const JSONFile file(R"({"id":"","data":{"slots":{},"polygons":[]}})");
		Shape::Collection::Builder builder;
		try
		{
			builder.load(file.path());
			FAIL() << "Empty Shape ID must throw";
		} catch (const spk::Exception &error)
		{
			EXPECT_NE(std::string(error.what()).find("$.id:"), std::string::npos);
			EXPECT_NE(std::string(error.what()).find("nonempty ID"), std::string::npos);
		}
	}

	TEST(VoxelShapeCollectionTest, RejectsDuplicateJSONEntries)
	{
		const JSONFile file(R"({"elements":[
			{"id":"same","data":{"slots":{},"polygons":[]}},
			{"id":"same","data":{"slots":{},"polygons":[]}}
		]})");
		Shape::Collection::Builder builder;
		EXPECT_THROW(builder.load(file.path()), spk::Exception);
		EXPECT_EQ(builder.build().size(), 0u);
	}

	TEST(VoxelShapeCollectionTest, InvalidBatchDoesNotAddEarlierShapes)
	{
		Shape::Collection::Builder builder;
		builder.add("existing", Shape::Builder{}.build());
		const JSONFile invalid(R"({"elements":[
			{"id":"valid","data":{"slots":{},"polygons":[]}},
			{"id":"invalid","data":{"slots":{},"polygons":[{"slot":"missing","vertices":[]}]}}
		]})");
		EXPECT_THROW(builder.load(invalid.path()), spk::Exception);
		const auto result = builder.build();
		EXPECT_EQ(result.size(), 1u);
		EXPECT_TRUE(result.contains("existing"));
		EXPECT_FALSE(result.contains("valid"));
	}

	TEST(VoxelDefinitionBuilderTest, UsesRegisteredShapeAndBuildsImmutableDefinitions)
	{
		const Shape::Collection shapes = shapeCollection();
		Definition::Builder builder;
		builder.bind(shapes);
		builder.setShape("cube");
		builder.setSlots({{"face", {"rock"}}});
		const Definition first = builder.build();
		builder.setSlots({{"face", {"glass"}}});
		const Definition second = builder.build();
		EXPECT_TRUE(first.shape() == shapes["cube"]);
		EXPECT_TRUE(second.shape() == shapes["cube"]);
		EXPECT_FALSE(first == second);
		EXPECT_EQ(first.semanticSlot("face"), "rock");
		EXPECT_EQ(second.semanticSlot("face"), "glass");
	}

	TEST(VoxelDefinitionBuilderTest, MissingShapeOrSlotsRejectBuild)
	{
		const Shape::Collection shapes = shapeCollection();
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
		const Shape::Collection shapes = shapeCollection();
		Definition::Builder builder;
		builder.bind(shapes);
		builder.setShape("cube");
		builder.setSlots({{"face", {"original"}}});
		const auto invalid = spk::JSON::Value::fromString(
			R"({"shape":"absent","slots":{"face":"bad"}})");
		EXPECT_THROW(builder.loadFromJSON(invalid), spk::Exception);
		EXPECT_EQ(builder.build().semanticSlot("face"), "original");
	}

	TEST(VoxelDefinitionBuilderTest, CollectionBindingOutlivesOriginalCollection)
	{
		Definition::Builder builder;
		{
			const Shape::Collection shapes = shapeCollection();
			builder.bind(shapes);
		}
		builder.setShape("slab");
		builder.setSlots({{"side", {"transparent", 0.75f}}});
		const Definition definition = builder.build();
		EXPECT_FLOAT_EQ(definition.slot("side").opacity, 0.75f);
	}

	TEST(VoxelDefinitionBuilderTest, EmptyCollectionRejectsUnknownIDsAndBindingEnablesJSON)
	{
		Definition::Builder builder;
		const auto json = spk::JSON::Value::fromString(
			R"({"shape":"cube","slots":{"face":"stone"}})");
		EXPECT_THROW(builder.setShape("cube"), spk::Exception);
		EXPECT_THROW(builder.loadFromJSON(json), spk::Exception);
		EXPECT_THROW((void)builder.build(), spk::Exception);
		const Shape::Collection shapes = shapeCollection();
		builder.bind(shapes);
		builder.loadFromJSON(json);
		EXPECT_EQ(builder.build().semanticSlot("face"), "stone");
	}

	TEST(VoxelDefinitionBuilderTest, RebindingRequiresReselectingShape)
	{
		const Shape::Collection shapes = shapeCollection();
		Definition::Builder builder;
		builder.bind(shapes);
		builder.setShape("cube");
		builder.setSlots({{"face", {"stone"}}});
		const Definition original = builder.build();
		builder.bind(Shape::Collection());
		EXPECT_THROW((void)builder.build(), spk::Exception);
		builder.setSlots({});
		EXPECT_THROW((void)builder.build(), spk::Exception);
		EXPECT_THROW(builder.setShape("cube"), spk::Exception);
		builder.bind(shapes);
		EXPECT_THROW((void)builder.build(), spk::Exception);
		builder.setShape("cube");
		builder.setSlots({{"face", {"stone"}}});
		EXPECT_TRUE(builder.build().shape() == original.shape());
	}

	TEST(VoxelDefinitionBuilderTest, ExplicitlySelectedEmptyShapeIsNotAir)
	{
		Shape::Collection::Builder shapeBuilder;
		shapeBuilder.add("empty", Shape::Builder{}.build());
		const Shape::Collection shapes = shapeBuilder.build();
		Definition::Builder builder;
		builder.bind(shapes);
		EXPECT_THROW((void)builder.build(), spk::Exception);
		builder.setShape("empty");
		const Definition result = builder.build();
		EXPECT_TRUE(result.shape().empty());
		EXPECT_TRUE(result.slots().empty());
		EXPECT_FALSE(result.isAir());
	}

	TEST(VoxelDefinitionCollectionTest, InvalidBatchPreservesEarlierDefinitions)
	{
		Definition::Collection::Builder builder;
		builder.bind(shapeCollection());
		const JSONFile original(
			R"({"id":42,"data":{"shape":"cube","slots":{"face":"stone"}}})");
		builder.load(original.path());
		const JSONFile invalid(R"({"elements":[
			{"id":43,"data":{"shape":"cube","slots":{"face":"dirt"}}},
			{"id":44,"data":{"shape":"unknown","slots":{"face":"glass"}}}
		]})");
		EXPECT_THROW(builder.load(invalid.path()), spk::Exception);
		const auto result = builder.build();
		EXPECT_EQ(result.size(), 2u);
		EXPECT_EQ(result.at(42).semanticSlot("face"), "stone");
		EXPECT_FALSE(result.contains(43));
	}

	TEST(VoxelDefinitionCollectionTest, LoadsNumericIDsThroughJSONCatalog)
	{
		const Shape::Collection shapes = shapeCollection();
		const JSONFile file(Definitions);
		Definition::Collection::Builder builder;
		builder.bind(shapes);
		builder.load(file.path());
		const Definition::Collection collection = builder.build();
		EXPECT_EQ(collection.size(), 3u);
		EXPECT_TRUE(collection[0].isAir());
		EXPECT_TRUE(collection[42].shape() == shapes["cube"]);
		EXPECT_EQ(collection[42].semanticSlot("face"), "stone");
		EXPECT_EQ(collection[7].semanticSlot("side"), "glass");
		EXPECT_FLOAT_EQ(collection[7].slot("side").opacity, 0.75f);
		EXPECT_THROW((void)collection[10], spk::Exception);
	}

	TEST(VoxelDefinitionCollectionTest, AirAccessIsConsistentAcrossEveryLookupMethod)
	{
		const Definition::Collection collection;
		const Definition *air = &Definition::air();
		EXPECT_TRUE(collection.contains(0));
		EXPECT_EQ(collection.tryGet(0), air);
		EXPECT_EQ(&collection.at(0), air);
		EXPECT_EQ(&collection[0], air);
		EXPECT_EQ(collection.size(), 1u);
	}

	TEST(VoxelDefinitionCollectionTest, UnknownAndRegisteredIDsMatchLookupContracts)
	{
		const Shape::Collection shapes = shapeCollection();
		const JSONFile file(Definitions);
		Definition::Collection::Builder builder;
		builder.bind(shapes);
		builder.load(file.path());
		const Definition::Collection collection = builder.build();
		EXPECT_EQ(collection.tryGet(42), &collection.at(42));
		EXPECT_EQ(collection.tryGet(7), &collection[7]);
		EXPECT_FALSE(collection.contains(999));
		EXPECT_EQ(collection.tryGet(999), nullptr);
		try
		{
			(void)collection.at(999);
			FAIL() << "Unknown Definition ID must throw";
		} catch (const spk::Exception &exception)
		{
			EXPECT_NE(std::string(exception.what()).find("999"), std::string::npos);
		}
		EXPECT_THROW((void)collection[999], spk::Exception);
	}

	TEST(VoxelDefinitionCollectionTest, JSONRejectsUnboundShapesAndUnknownShapeKeys)
	{
		const JSONFile file(Definitions);
		Definition::Collection::Builder unbound;
		EXPECT_THROW(unbound.load(file.path()), spk::Exception);
		Shape::Collection shapes;
		Definition::Collection::Builder builder;
		builder.bind(shapes);
		EXPECT_THROW(builder.load(file.path()), spk::Exception);
	}

	TEST(VoxelDefinitionCollectionTest, JSONRejectsReservedAndOversizedIDs)
	{
		const Shape::Collection shapes = shapeCollection();
		Definition::Collection::Builder builder;
		builder.bind(shapes);
		const JSONFile air(R"({"id":0,"data":{"shape":"cube","slots":{"face":"stone"}}})");
		const JSONFile oversized(R"({"id":536870912,"data":{"shape":"cube","slots":{"face":"stone"}}})");
		try
		{
			builder.load(air.path());
			FAIL() << "Reserved Definition ID must throw";
		} catch (const spk::Exception &error)
		{
			EXPECT_NE(std::string(error.what()).find("$.id:"), std::string::npos);
			EXPECT_NE(std::string(error.what()).find("fit 29 bits"), std::string::npos);
		}
		EXPECT_THROW(builder.load(oversized.path()), spk::Exception);
		EXPECT_EQ(builder.build().size(), 1u);
	}

	TEST(VoxelDefinitionCollectionTest, ProgrammaticAddChecksBoundsAndAir)
	{
		const Shape::Collection shapes = shapeCollection();
		Definition::Builder builder;
		builder.bind(shapes);
		builder.setShape("cube");
		builder.setSlots({{"face", {"stone"}}});
		const Definition stone = builder.build();
		Definition::Collection::Builder builderCollection;
		builderCollection.add(13, stone);
		const Definition::Collection collection = builderCollection.build();
		EXPECT_TRUE(collection[13] == stone);
		EXPECT_THROW(builderCollection.add(13, stone), spk::Exception);
		EXPECT_THROW(builderCollection.add(0, stone), spk::Exception);
		EXPECT_THROW(builderCollection.add(14, Definition::air()), spk::Exception);
		EXPECT_EQ(collection.size(), 2u);
	}

	TEST(VoxelShapeCollectionTest, PublishedSnapshotsShareDataButIgnoreLaterBuilderChanges)
	{
		Shape::Collection::Builder builder;
		const Shape cube = Shape::Builder{}.build();
		builder.add("cube", cube);
		const Shape::Collection first = builder.build();
		const Shape::Collection copy = first;
		EXPECT_EQ(&first["cube"], &copy["cube"]);
		builder.add("slab", Shape::Builder{}.build());
		const Shape::Collection second = builder.build();
		EXPECT_FALSE(first.contains("slab"));
		EXPECT_TRUE(second.contains("slab"));
		EXPECT_TRUE(first["cube"] == second["cube"]);
	}

	TEST(VoxelDefinitionCollectionTest, PublishedSnapshotsShareDataButIgnoreLaterBuilderChanges)
	{
		const Shape::Collection shapes = shapeCollection();
		Definition::Builder definitionBuilder;
		definitionBuilder.bind(shapes);
		definitionBuilder.setShape("cube");
		definitionBuilder.setSlots({{"face", {"rock"}}});
		Definition::Collection::Builder builder;
		builder.add(9, definitionBuilder.build());
		const Definition::Collection first = builder.build();
		const Definition::Collection copy = first;
		EXPECT_EQ(&first[9], &copy[9]);
		definitionBuilder.setSlots({{"face", {"glass"}}});
		builder.add(20, definitionBuilder.build());
		const Definition::Collection second = builder.build();
		EXPECT_FALSE(first.contains(20));
		EXPECT_TRUE(second.contains(20));
		EXPECT_TRUE(first[9] == second[9]);
	}

	TEST(VoxelDefinitionCollectionTest, LoadedDefinitionsBindToVolumeByNumericID)
	{
		const Shape::Collection shapes = shapeCollection();
		const JSONFile file(Definitions);
		Definition::Collection::Builder collectionBuilder;
		collectionBuilder.bind(shapes);
		collectionBuilder.load(file.path());
		const Definition::Collection collection = collectionBuilder.build();
		Volume::Builder builder({2, 1, 1});
		builder.bind(collection);
		builder.set({1, 0, 0}, 42);
		const Volume volume = builder.build();
		EXPECT_TRUE(volume.at({0, 0, 0}).isAir());
		EXPECT_TRUE(volume.at({1, 0, 0}).definition() == collection[42]);
	}
}
