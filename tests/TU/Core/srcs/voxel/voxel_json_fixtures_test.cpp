#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "container/json/object.hpp"
#include "exception.hpp"
#include "voxel/cell.hpp"

namespace
{
	using Shape = spk::Voxel::Shape;
	using Cell = spk::Voxel::Cell;
	using Definition = spk::Voxel::Definition;

	struct Fixture
	{
		const char *name;
		std::size_t polygonCount;
		std::size_t outerCount;
		std::size_t innerCount;
	};

	constexpr std::array<Fixture, 4> Fixtures = {
		Fixture{"cube", 6, 6, 0},
		{"slab", 6, 5, 1},
		{"slope", 5, 4, 1},
		{"stair", 10, 8, 2}};

	[[nodiscard]] spk::JSON::Value fixtureJSON(const std::string &name)
	{
		const auto path = std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (name + ".json");
		return spk::JSON::Value::loadFromFile(path);
	}

	[[nodiscard]] Shape fixtureShape(const std::string &name)
	{
		Shape::Builder builder(fixtureJSON(name));
		return builder.build();
	}

	[[nodiscard]] std::array<Cell, 8> orientations(const Definition &definition)
	{
		using Horizontal = Cell::HorizontalOrientation;
		using Vertical = Cell::VerticalOrientation;
		return {
			Cell(definition, Horizontal::XPositive, Vertical::YPositive),
			Cell(definition, Horizontal::ZNegative, Vertical::YPositive),
			Cell(definition, Horizontal::XNegative, Vertical::YPositive),
			Cell(definition, Horizontal::ZPositive, Vertical::YPositive),
			Cell(definition, Horizontal::XPositive, Vertical::YNegative),
			Cell(definition, Horizontal::ZNegative, Vertical::YNegative),
			Cell(definition, Horizontal::XNegative, Vertical::YNegative),
			Cell(definition, Horizontal::ZPositive, Vertical::YNegative)};
	}

	TEST(VoxelShapeFixtureTest, EachJSONFileContainsExactlyOneIndependentlyLoadableShape)
	{
		for (const Fixture &fixture : Fixtures)
		{
			SCOPED_TRACE(fixture.name);
			const auto json = fixtureJSON(fixture.name);
			ASSERT_TRUE(json.isObject());
			ASSERT_EQ(json.at("id").as<std::string>(), fixture.name);
			EXPECT_FALSE(json.contains("elements"));
			EXPECT_TRUE(json.at("polygons").isArray());

			const Shape first = fixtureShape(fixture.name);
			const Shape second = fixtureShape(fixture.name);
			EXPECT_FALSE(first == second);
			EXPECT_TRUE(first == Shape(first));
			EXPECT_EQ(first.polygons().size(), fixture.polygonCount);
			EXPECT_EQ(first.slots().size(), 3u);
		}
	}

	TEST(VoxelShapeFixtureTest, LegacyGeometryIsPlanarConvexAndCorrectlyClassified)
	{
		for (const Fixture &fixture : Fixtures)
		{
			SCOPED_TRACE(fixture.name);
			const Shape shape = fixtureShape(fixture.name);
			std::size_t outerCount = 0;
			std::size_t innerCount = 0;
			for (const auto &polygon : shape.polygons())
			{
				ASSERT_GE(polygon.vertices.size(), 3u);
				EXPECT_FLOAT_EQ(shape.slot(polygon.slotKey).opacity, 1.0f);
				const auto &vertices = polygon.vertices;
				const auto origin = vertices[0].position;
				const auto edgeA = vertices[1].position - origin;
				const auto edgeB = vertices[2].position - origin;
				EXPECT_GT(edgeA.cross(edgeB).dot(polygon.geometricNormal), 0.0f);
				EXPECT_NEAR(polygon.geometricNormal.length(), 1.0, 1.0e-5);
				for (const auto &vertex : vertices)
				{
					EXPECT_GE(vertex.position.x, 0.0f);
					EXPECT_LE(vertex.position.x, 1.0f);
					EXPECT_GE(vertex.position.y, 0.0f);
					EXPECT_LE(vertex.position.y, 1.0f);
					EXPECT_GE(vertex.position.z, 0.0f);
					EXPECT_LE(vertex.position.z, 1.0f);
					EXPECT_NEAR((vertex.position - origin).dot(polygon.geometricNormal), 0.0f, 1.0e-5);
					ASSERT_TRUE(vertex.normal.has_value());
					EXPECT_NEAR(vertex.normal->length(), 1.0, 1.0e-5);
				}
				if (polygon.shell == Shape::Shell::Outer)
				{
					++outerCount;
					EXPECT_TRUE(polygon.boundary.has_value());
				}
				else
				{
					++innerCount;
					EXPECT_FALSE(polygon.boundary.has_value());
				}
			}
			EXPECT_EQ(outerCount, fixture.outerCount);
			EXPECT_EQ(innerCount, fixture.innerCount);
		}
	}

	TEST(VoxelShapeFixtureTest, EveryShapeSupportsEightOrientedCellsWithoutUVOrWindingLoss)
	{
		for (const Fixture &fixture : Fixtures)
		{
			SCOPED_TRACE(fixture.name);
			const Shape shape = fixtureShape(fixture.name);
			Definition::SlotMapping mapping;
			for (const auto &slot : shape.slots())
			{
				mapping.emplace(slot.name, fixture.name + std::string(".") + slot.name);
			}
			const Definition definition(shape, std::move(mapping));
			for (const auto &cell : orientations(definition))
			{
				const auto oriented = cell.orientedPolygons();
				ASSERT_EQ(oriented.size(), shape.polygons().size());
				for (std::size_t polygonIndex = 0; polygonIndex < oriented.size(); ++polygonIndex)
				{
					const auto &original = shape.polygons()[polygonIndex];
					const auto &result = oriented[polygonIndex];
					ASSERT_EQ(result.vertices.size(), original.vertices.size());
					EXPECT_EQ(result.slotKey, original.slotKey);
					EXPECT_EQ(result.shell, original.shell);
					EXPECT_FALSE(definition.semanticSlot(result.slotKey).empty());
					const bool flipped = cell.verticalOrientation() == Cell::VerticalOrientation::YNegative;
					for (std::size_t vertexIndex = 0; vertexIndex < result.vertices.size(); ++vertexIndex)
					{
						const std::size_t sourceIndex = flipped ? result.vertices.size() - 1 - vertexIndex : vertexIndex;
						const auto &source = original.vertices[sourceIndex];
						const auto &transformed = result.vertices[vertexIndex];
						EXPECT_EQ(transformed.uv, source.uv);
						const auto expected = cell.transformPosition(source.position);
						EXPECT_NEAR((transformed.position - expected).length(), 0.0, 1.0e-6);
						EXPECT_NEAR((*transformed.normal - cell.transformNormal(*source.normal)).length(), 0.0, 1.0e-6);
					}
					const auto edgeA = result.vertices[1].position - result.vertices[0].position;
					const auto edgeB = result.vertices[2].position - result.vertices[0].position;
					EXPECT_GT(edgeA.cross(edgeB).dot(result.geometricNormal), 0.0f);
				}
			}
		}
	}

	TEST(VoxelShapeFixtureTest, JSONLoaderPreservesExplicitUVAndLightingNormal)
	{
		const auto json = spk::JSON::Value::fromString(
			R"({"slots":{"surface":0.75},"polygons":[{"slot":"surface","vertices":[{"position":[0,0,0],"uv":[0.25,0.75],"normal":[0,0,1]},{"position":[1,0,0],"uv":[1,0],"normal":[0,0,1]},{"position":[0,1,0],"uv":[0,1],"normal":[0,0,1]}]}]})");
		const Shape shape = Shape::Builder(json).build();
		ASSERT_EQ(shape.polygons().size(), 1u);
		EXPECT_FLOAT_EQ(shape.slot("surface").opacity, 0.75f);
		EXPECT_EQ(shape.polygons()[0].vertices[0].uv, spk::Vector2(0.25f, 0.75f));
		EXPECT_EQ(shape.polygons()[0].vertices[0].normal.value(), spk::Vector3(0, 0, 1));
	}

	TEST(VoxelShapeFixtureTest, InvalidJSONDoesNotPartiallyModifyExistingBuilder)
	{
		Shape::Builder builder;
		builder.addSlot("existing");
		const auto malformed = spk::JSON::Value::fromString(
			R"({"slots":{"temporary":1},"polygons":[{"slot":"temporary","vertices":[[0,0,0],[1,0,0],[0,1,2]]}]})");
		EXPECT_THROW(builder.addFromJSON(malformed), spk::Exception);
		builder.addPolygon("existing", {
			{{0, 0, 0}, {}, std::nullopt},
			{{1, 0, 0}, {}, std::nullopt},
			{{0, 1, 0}, {}, std::nullopt}});
		const Shape shape = builder.build();
		EXPECT_EQ(shape.slots().size(), 1u);
		EXPECT_EQ(shape.polygons().size(), 1u);
		EXPECT_EQ(shape.polygons()[0].slotKey, "existing");
	}

	TEST(VoxelShapeFixtureTest, RejectsLegacyMultiShapeCollectionAsOneShape)
	{
		const auto multiple = spk::JSON::Value::fromString(
			R"({"elements":[{"id":"cube","data":{"polygons":[]}},{"id":"slab","data":{"polygons":[]}}]})");
		EXPECT_THROW((void)Shape::Builder(multiple), spk::Exception);
	}
}
