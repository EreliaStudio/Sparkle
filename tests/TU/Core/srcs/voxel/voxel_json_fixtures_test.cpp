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
		std::size_t triangleCount;
		std::size_t outerCount;
		std::size_t innerCount;
	};

	constexpr std::array<Fixture, 4> Fixtures = {
		Fixture{"cube", 6, 12, 12, 0},
		{"slab", 6, 12, 10, 2},
		{"slope", 5, 8, 6, 2},
		{"stair", 10, 20, 16, 4}};

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
			EXPECT_EQ(json.at("polygons").asArray().size(), fixture.polygonCount);

			const Shape first = fixtureShape(fixture.name);
			const Shape second = fixtureShape(fixture.name);
			EXPECT_FALSE(first == second);
			EXPECT_TRUE(first == Shape(first));
			EXPECT_EQ(first.instance().triangles().size(), fixture.triangleCount);
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
			for (const auto &polygon : shape.instance().triangles())
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
				mapping.emplace(slot.name, Definition::Slot{fixture.name + std::string(".") + slot.name});
			}
			const Definition definition(shape, std::move(mapping));
			for (const auto &cell : orientations(definition))
			{
				EXPECT_TRUE(cell.definition() == definition);
				EXPECT_TRUE(cell.definition().shape() == shape);
				EXPECT_EQ(cell.definition().shape().instance().triangles().size(), fixture.triangleCount);
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
			const auto &original = shape.instance().triangles();
			const Definition definition(shape, [&] {
				Definition::SlotMapping mapping;
				for (const auto &slot : shape.slots())
				{
					mapping.emplace(slot.name, Definition::Slot{slot.name});
				}
				return mapping;
			}());
			for (const Cell &cell : orientations(definition))
			{
				const Shape::Orientation orientation{cell.horizontalOrientation(), cell.verticalOrientation()};
				const auto &triangles = shape.instance(orientation).triangles();
				ASSERT_EQ(triangles.size(), original.size());
				for (std::size_t i = 0; i < triangles.size(); ++i)
				{
					const auto &triangle = triangles[i];
					EXPECT_EQ(triangle.vertices.size(), 3u);
					EXPECT_EQ(triangle.slotKey, original[i].slotKey);
					EXPECT_EQ(triangle.shell, original[i].shell);
					for (const auto &vertex : triangle.vertices)
					{
						bool foundUV = false;
						for (const auto &source : original)
						{
							for (const auto &candidate : source.vertices)
							{
								foundUV |= candidate.uv == vertex.uv;
							}
						}
						EXPECT_TRUE(foundUV);
						ASSERT_TRUE(vertex.normal.has_value());
						EXPECT_NEAR(vertex.normal->length(), 1.0, 1.0e-5);
					}
					const auto a = triangle.vertices[1].position - triangle.vertices[0].position;
					const auto b = triangle.vertices[2].position - triangle.vertices[0].position;
					EXPECT_GT(a.cross(b).dot(triangle.geometricNormal), 0.0f);
				}
			}
		}
	}

	TEST(VoxelShapeFixtureTest, EveryFixturePrecomputesTriangleFansForAllOrientations)
	{
		for (const Fixture &fixture : Fixtures)
		{
			const Shape shape = fixtureShape(fixture.name);
			const auto sourceJSON = fixtureJSON(fixture.name);
			const auto &originals = sourceJSON.at("polygons").asArray();
			for (unsigned flipped = 0; flipped < 2; ++flipped)
			{
				for (unsigned degrees = 0; degrees < 360; degrees += 90)
				{
					const Shape::Orientation orientation{
						static_cast<Shape::HorizontalOrientation>(degrees),
						flipped ? Shape::VerticalOrientation::YNegative : Shape::VerticalOrientation::YPositive};
					const auto &triangles = shape.instance(orientation).triangles();
					std::size_t offset = 0;
					for (const auto &source : originals)
					{
						const std::size_t count = source.at("vertices").asArray().size() - 2;
						ASSERT_LE(offset + count, triangles.size());
						for (std::size_t i = 0; i < count; ++i)
						{
							const auto &triangle = triangles[offset + i];
							EXPECT_EQ(triangle.slotKey, source.at("slot").as<std::string>());
							ASSERT_EQ(triangle.vertices.size(), 3u);
							if (i > 0)
							{
								EXPECT_EQ(triangle.vertices[0].position, triangles[offset].vertices[0].position);
								EXPECT_EQ(triangle.vertices[1].position, triangles[offset + i - 1].vertices[2].position);
							}
						}
						offset += count;
					}
					EXPECT_EQ(offset, triangles.size());
					EXPECT_EQ(offset, fixture.triangleCount);
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
				const auto &triangles = cube.instance({horizontal, vertical}).triangles();
				std::array<std::size_t, 6> boundaries{};
				ASSERT_EQ(triangles.size(), 12u);
				for (const auto &triangle : triangles)
				{
					ASSERT_TRUE(triangle.boundary.has_value());
					++boundaries[static_cast<std::size_t>(*triangle.boundary)];
				}
				for (const auto count : boundaries)
				{
					EXPECT_EQ(count, 2u);
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
		ASSERT_EQ(shape.instance().triangles().size(), 1u);
		EXPECT_EQ(shape.slot("surface").name, "surface");
		EXPECT_EQ(shape.instance().triangles()[0].vertices[0].uv, spk::Vector2(0.25f, 0.75f));
		EXPECT_EQ(shape.instance().triangles()[0].vertices[0].normal.value(), spk::Vector3(0, 0, 1));
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
		EXPECT_EQ(shape.instance().triangles().size(), 1u);
		EXPECT_EQ(shape.instance().triangles()[0].slotKey, "existing");
	}

	TEST(VoxelShapeFixtureTest, RejectsLegacyMultiShapeCollectionAsOneShape)
	{
		const auto multiple = spk::JSON::Value::fromString(
			R"({"elements":[{"id":"cube","data":{"polygons":[]}},{"id":"slab","data":{"polygons":[]}}]})");
		Shape::Builder builder;
		EXPECT_THROW(builder.addFromJSON(multiple), spk::Exception);
	}
}
