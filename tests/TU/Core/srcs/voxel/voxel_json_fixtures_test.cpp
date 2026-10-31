#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
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
		Shape::Builder builder;
		builder.addFromJSON(fixtureJSON(name));
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
			EXPECT_EQ(first.instance().polygons().size(), fixture.polygonCount);
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
			for (const auto &polygon : shape.instance().polygons())
			{
				ASSERT_GE(polygon.vertices.size(), 3u);
				EXPECT_EQ(shape.slot(polygon.slotKey).name, polygon.slotKey);
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

	TEST(VoxelShapeFixtureTest, EachJSONShapeCanBeReferencedByEightOrientedCells)
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
				EXPECT_TRUE(cell.definition() == definition);
				EXPECT_TRUE(cell.definition().shape() == shape);
				EXPECT_EQ(cell.definition().shape().instance().polygons().size(), fixture.polygonCount);
				for (const auto &slot : shape.slots())
				{
					EXPECT_EQ(cell.definition().semanticSlot(slot.name), fixture.name + std::string(".") + slot.name);
				}
			}
		}
	}

	TEST(VoxelShapeFixtureTest, EightInstancesRetainGeometryAndUVWithConsistentWinding)
	{
		for (const Fixture &fixture : Fixtures)
		{
			SCOPED_TRACE(fixture.name);
			const Shape shape = fixtureShape(fixture.name);
			const auto &original = shape.instance().polygons();
			const Definition definition(shape, [&] {
				Definition::SlotMapping mapping;
				for (const auto &slot : shape.slots())
				{
					mapping.emplace(slot.name, slot.name);
				}
				return mapping;
			}());
			for (const Cell &cell : orientations(definition))
			{
				const Shape::Orientation orientation{cell.horizontalOrientation(), cell.verticalOrientation()};
				const auto &polygons = shape.instance(orientation).polygons();
				ASSERT_EQ(polygons.size(), original.size());
				const bool flipped = cell.verticalOrientation() == Cell::VerticalOrientation::YNegative;
				for (std::size_t polygonIndex = 0; polygonIndex < polygons.size(); ++polygonIndex)
				{
					const auto &polygon = polygons[polygonIndex];
					const auto &source = original[polygonIndex];
					ASSERT_EQ(polygon.vertices.size(), source.vertices.size());
					EXPECT_EQ(polygon.slotKey, source.slotKey);
					EXPECT_EQ(polygon.shell, source.shell);
					for (std::size_t index = 0; index < polygon.vertices.size(); ++index)
					{
						const std::size_t sourceIndex = flipped ? source.vertices.size() - 1 - index : index;
						const auto &vertex = polygon.vertices[index];
						EXPECT_EQ(vertex.uv, source.vertices[sourceIndex].uv);
						ASSERT_TRUE(vertex.normal.has_value());
						EXPECT_NEAR(vertex.normal->length(), 1.0, 1.0e-5);
					}
					const auto a = polygon.vertices[1].position - polygon.vertices[0].position;
					const auto b = polygon.vertices[2].position - polygon.vertices[0].position;
					EXPECT_GT(a.cross(b).dot(polygon.geometricNormal), 0.0f);
				}
			}
		}
	}

	TEST(VoxelShapeFixtureTest, CubeKeepsSixDistinctBoundariesInEveryOrientation)
	{
		const Shape cube = fixtureShape("cube");
		for (unsigned flipped = 0; flipped != 2; ++flipped)
		{
			for (unsigned degrees = 0; degrees != 360; degrees += 90)
			{
				const auto horizontal = static_cast<Shape::HorizontalOrientation>(degrees);
				const auto vertical = flipped ? Shape::VerticalOrientation::YNegative : Shape::VerticalOrientation::YPositive;
				const auto &polygons = cube.instance({horizontal, vertical}).polygons();
				std::array<bool, 6> boundaries{};
				ASSERT_EQ(polygons.size(), 6u);
				for (const auto &polygon : polygons)
				{
					ASSERT_TRUE(polygon.boundary.has_value());
					boundaries[static_cast<std::size_t>(*polygon.boundary)] = true;
				}
				for (bool present : boundaries)
				{
					EXPECT_TRUE(present);
				}
			}
		}
	}

	TEST(VoxelShapeFixtureTest, JSONLoaderPreservesExplicitUVAndLightingNormal)
	{
		const auto json = spk::JSON::Value::fromString(
			R"({"slots":{"surface":null},"polygons":[{"slot":"surface","vertices":[{"position":[0,0,0],"uv":[0.25,0.75],"normal":[0,0,1]},{"position":[1,0,0],"uv":[1,0],"normal":[0,0,1]},{"position":[0,1,0],"uv":[0,1],"normal":[0,0,1]}]}]})");
		Shape::Builder builder;
		builder.addFromJSON(json);
		const Shape shape = builder.build();
		ASSERT_EQ(shape.instance().polygons().size(), 1u);
		EXPECT_EQ(shape.slot("surface").name, "surface");
		EXPECT_EQ(shape.instance().polygons()[0].vertices[0].uv, spk::Vector2(0.25f, 0.75f));
		EXPECT_EQ(shape.instance().polygons()[0].vertices[0].normal.value(), spk::Vector3(0, 0, 1));
	}

	TEST(VoxelShapeFixtureTest, InvalidJSONDoesNotPartiallyModifyExistingBuilder)
	{
		Shape::Builder builder;
		builder.addSlot("existing");
		const auto malformed = spk::JSON::Value::fromString(
			R"({"slots":{"temporary":1},"polygons":[{"slot":"temporary","vertices":[[0,0,0],[1,0,0],[0,1,2]]}]})");
		EXPECT_THROW(builder.addFromJSON(malformed), spk::Exception);
		std::vector<Shape::Vertex> vertices;
		vertices.push_back({{0, 0, 0}, {}, std::nullopt});
		vertices.push_back({{1, 0, 0}, {}, std::nullopt});
		vertices.push_back({{0, 1, 0}, {}, std::nullopt});
		builder.addPolygon("existing", std::move(vertices));
		const Shape shape = builder.build();
		EXPECT_EQ(shape.slots().size(), 1u);
		EXPECT_EQ(shape.instance().polygons().size(), 1u);
		EXPECT_EQ(shape.instance().polygons()[0].slotKey, "existing");
	}

	TEST(VoxelShapeFixtureTest, RejectsLegacyMultiShapeCollectionAsOneShape)
	{
		const auto multiple = spk::JSON::Value::fromString(
			R"({"elements":[{"id":"cube","data":{"polygons":[]}},{"id":"slab","data":{"polygons":[]}}]})");
		Shape::Builder builder;
		EXPECT_THROW(builder.addFromJSON(multiple), spk::Exception);
	}
}
