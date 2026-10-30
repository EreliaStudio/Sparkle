#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "diagnostics/logger.hpp"
#include "exception.hpp"
#include "voxel/cell.hpp"

namespace
{
	using Shape = spk::Voxel::Shape;
	using Definition = spk::Voxel::Definition;
	using Cell = spk::Voxel::Cell;

	Shape::Vertex vertex(float x, float y, float z, float u = 0.0f, float v = 0.0f)
	{
		return {{x, y, z}, {u, v}, std::nullopt};
	}

	std::vector<Shape::Vertex> square()
	{
		return {
			vertex(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			vertex(0.0f, 0.0f, 1.0f, 1.0f, 0.0f),
			vertex(0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
			vertex(0.0f, 1.0f, 0.0f, 0.0f, 1.0f)};
	}

	Shape makeSquare()
	{
		Shape::Builder builder;
		builder.addSlot("wall");
		builder.addPolygon("wall", square());
		return builder.build();
	}

	void expectVectorNear(spk::Vector3 actual, spk::Vector3 expected)
	{
		EXPECT_NEAR(actual.x, expected.x, 1.0e-6);
		EXPECT_NEAR(actual.y, expected.y, 1.0e-6);
		EXPECT_NEAR(actual.z, expected.z, 1.0e-6);
	}

	static_assert(std::is_copy_constructible_v<Shape>);
	static_assert(std::is_copy_constructible_v<Definition>);
	static_assert(std::is_copy_constructible_v<Cell>);
	static_assert(std::is_same_v<decltype(std::declval<const Shape &>().polygons()), const std::vector<Shape::Polygon> &>);
	static_assert(std::is_same_v<decltype(std::declval<const Definition &>().shape()), const Shape &>);

	TEST(VoxelShapeTest, EmptyIsValidAndDistinctFromOtherConstructions)
	{
		Shape::Builder firstBuilder;
		Shape::Builder secondBuilder;
		const Shape first = firstBuilder.build();
		const Shape second = secondBuilder.build();
		EXPECT_TRUE(first.empty());
		EXPECT_TRUE(first.slots().empty());
		EXPECT_FALSE(first.sameIdentity(second));
		EXPECT_TRUE(first.sameIdentity(Shape(first)));
		EXPECT_THROW(firstBuilder.build(), spk::Exception);
		EXPECT_THROW(firstBuilder.addSlot("later"), spk::Exception);
	}

	TEST(VoxelShapeTest, ValidPolygonsRetainSlotsWindingLightingNormalsAndClassification)
	{
		Shape::Builder builder;
		builder.addSlot("wall");
		builder.addSlot("glass", 0.25f);
		builder.addPolygon("wall", square());
		builder.addPolygon("glass", {vertex(0.2f, 0.1f, 0.2f), vertex(0.8f, 0.1f, 0.2f), vertex(0.5f, 0.7f, 0.2f)});
		const Shape shape = builder.build();
		ASSERT_EQ(shape.polygons().size(), 2u);
		EXPECT_FLOAT_EQ(shape.slot("wall").opacity, 1.0f);
		EXPECT_FLOAT_EQ(shape.slot("glass").opacity, 0.25f);
		EXPECT_EQ(shape.polygons()[0].shell, Shape::Shell::Outer);
		EXPECT_EQ(shape.polygons()[0].boundary, Shape::Face::NegativeX);
		EXPECT_EQ(shape.polygons()[1].shell, Shape::Shell::Inner);
		EXPECT_FALSE(shape.polygons()[1].boundary.has_value());
		EXPECT_EQ(shape.polygons()[0].geometricNormal, spk::Vector3(-1.0f, 0.0f, 0.0f));
		EXPECT_EQ(shape.polygons()[0].vertices[2].normal.value(), spk::Vector3(-1.0f, 0.0f, 0.0f));
	}

	TEST(VoxelShapeTest, BuilderValidationIsAtomicForInvalidSlotsAndPolygons)
	{
		Shape::Builder builder;
		EXPECT_THROW(builder.addSlot("", 1.0f), spk::Exception);
		EXPECT_THROW(builder.addSlot("a", -0.1f), spk::Exception);
		EXPECT_THROW(builder.addSlot("a", std::numeric_limits<float>::quiet_NaN()), spk::Exception);
		builder.addSlot("a");
		EXPECT_THROW(builder.addSlot("a"), spk::Exception);
		EXPECT_THROW(builder.addPolygon("missing", square()), spk::Exception);
		EXPECT_THROW(builder.addPolygon("a", {vertex(0, 0, 0), vertex(0, 1, 0)}), spk::Exception);
		EXPECT_THROW(builder.addPolygon("a", {vertex(0, 0, 0), vertex(0, 0, 0), vertex(0, 1, 0)}), spk::Exception);
		EXPECT_THROW(builder.addPolygon("a", {vertex(0, 0, 0), vertex(0, 0, 1), vertex(0, 0, 0.5f)}), spk::Exception);
		EXPECT_THROW(builder.addPolygon("a", {vertex(-0.1f, 0, 0), vertex(0, 0, 1), vertex(0, 1, 1)}), spk::Exception);
		EXPECT_THROW(builder.addPolygon("a", {vertex(0, 0, 0), vertex(0, 0, 1), vertex(0, 1, 1), vertex(0.1f, 1, 0)}), spk::Exception);
		const std::vector<Shape::Vertex> concave = {vertex(0, 0, 0), vertex(1, 0, 0), vertex(0.5f, 0.25f, 0), vertex(1, 1, 0), vertex(0, 1, 0)};
		EXPECT_THROW(builder.addPolygon("a", concave), spk::Exception);
		const std::vector<Shape::Vertex> bowtie = {vertex(0, 0, 0), vertex(1, 1, 0), vertex(0, 1, 0), vertex(1, 0, 0)};
		EXPECT_THROW(builder.addPolygon("a", bowtie), spk::Exception);
		const std::vector<Shape::Vertex> infiniteUV = {vertex(0, 0, 0), vertex(0, 0, 1, std::numeric_limits<float>::infinity(), 0), vertex(0, 1, 1)};
		EXPECT_THROW(builder.addPolygon("a", infiniteUV), spk::Exception);
		auto invalidNormal = square();
		invalidNormal[0].normal = spk::Vector3{};
		EXPECT_THROW(builder.addPolygon("a", invalidNormal), spk::Exception);
		const Shape shape = builder.build();
		EXPECT_EQ(shape.slots().size(), 1u);
		EXPECT_TRUE(shape.polygons().empty());
	}

	TEST(VoxelShapeTest, UVOutOfRangeWarnsOnceAtConstructionAndRetainsCoordinates)
	{
		int warnings = 0;
		auto contract = spk::logger.subscribeToEntry(
			[&](const spk::Logger::Level &level, const std::string &message) {
				if (level == spk::Logger::Level::Warning &&
					message.find("Voxel Shape polygon") != std::string::npos)
				{
					++warnings;
				}
			});
		Shape::Builder builder;
		builder.addSlot("wall");
		auto vertices = square();
		vertices[0].uv = {-1.0f, 2.0f};
		builder.addPolygon("wall", vertices);
		const Shape shape = builder.build();
		ASSERT_EQ(warnings, 1);
		EXPECT_EQ(shape.polygons()[0].vertices[0].uv, spk::Vector2(-1.0f, 2.0f));
		(void)shape.polygons();
		EXPECT_EQ(warnings, 1);
	}

	TEST(VoxelDefinitionTest, CopiesShareIdentityButNewBuildsDoNot)
	{
		const Shape shape = makeSquare();
		const Shape identical = makeSquare();
		EXPECT_FALSE(shape.sameIdentity(identical));
		const Definition first(shape, {{"wall", "stone"}});
		const Definition copied = first;
		const Definition second(shape, {{"wall", "glass"}});
		const Definition third(identical, {{"wall", "stone"}});
		EXPECT_TRUE(first.sameIdentity(copied));
		EXPECT_FALSE(first.sameIdentity(second));
		EXPECT_FALSE(first.sameIdentity(third));
		EXPECT_TRUE(first.shape().sameIdentity(second.shape()));
		EXPECT_EQ(first.semanticSlot("wall"), "stone");
		EXPECT_EQ(second.semanticSlot("wall"), "glass");
		EXPECT_EQ(first.shape().polygons().size(), 1u);
		EXPECT_THROW(first.semanticSlot("none"), spk::Exception);
	}

	TEST(VoxelDefinitionTest, SlotMappingsMustBeCompleteAndValid)
	{
		const Shape shape = makeSquare();
		EXPECT_THROW((Definition(shape, {})), spk::Exception);
		EXPECT_THROW((Definition(shape, {{"missing", "x"}})), spk::Exception);
		EXPECT_THROW((Definition(shape, {{"wall", ""}})), spk::Exception);
		EXPECT_NO_THROW((Definition(shape, {{"wall", "stone"}})));
	}

	TEST(VoxelCellTest, DefaultAirIsValidAndDoesNotOcclude)
	{
		const Cell air;
		const Cell explicitAir(Definition::air(), Cell::Orientation::FlippedY270);
		EXPECT_TRUE(air.isAir());
		EXPECT_TRUE(explicitAir.isAir());
		EXPECT_TRUE(air.definition().sameIdentity(Definition::air()));
		EXPECT_TRUE(air.definition().shape().polygons().empty());
		EXPECT_TRUE(explicitAir.orientedPolygons().empty());
		EXPECT_EQ(air.orientation(), Cell::Orientation::Y0);
	}

	TEST(VoxelCellTest, EightOrientationsTransformPositionsAndNormals)
	{
		const Definition definition(makeSquare(), {{"wall", "stone"}});
		using O = Cell::Orientation;
		const std::array orientations = {
			O::Y0, O::Y90, O::Y180, O::Y270, O::FlippedY0, O::FlippedY90, O::FlippedY180, O::FlippedY270};
		const std::array<spk::Vector3, 8> positions = {
			spk::Vector3{0.2f, 0.3f, 0.4f},
			{0.4f, 0.3f, 0.8f},
			{0.8f, 0.3f, 0.6f},
			{0.6f, 0.3f, 0.2f},
			{0.2f, 0.7f, 0.4f},
			{0.4f, 0.7f, 0.8f},
			{0.8f, 0.7f, 0.6f},
			{0.6f, 0.7f, 0.2f}};
		const std::array<spk::Vector3, 8> normals = {
			spk::Vector3{1, 2, 3},
			{3, 2, -1},
			{-1, 2, -3},
			{-3, 2, 1},
			{1, -2, 3},
			{3, -2, -1},
			{-1, -2, -3},
			{-3, -2, 1}};
		for (std::size_t index = 0; index < orientations.size(); ++index)
		{
			const Cell cell(definition, orientations[index]);
			expectVectorNear(cell.transformPosition({0.2f, 0.3f, 0.4f}), positions[index]);
			expectVectorNear(cell.transformNormal({1, 2, 3}), normals[index]);
			EXPECT_TRUE(cell.definition().sameIdentity(definition));
		}
		EXPECT_THROW((Cell(definition, static_cast<O>(255))), spk::Exception);
	}

	TEST(VoxelCellTest, WindingUVAndFrontFacesSurviveEveryVerticalFlip)
	{
		const Definition definition(makeSquare(), {{"wall", "stone"}});
		for (unsigned value = 0; value < 8; ++value)
		{
			const Cell cell(definition, static_cast<Cell::Orientation>(value));
			const auto polygons = cell.orientedPolygons();
			ASSERT_EQ(polygons.size(), 1u);
			const auto &polygon = polygons.front();
			ASSERT_EQ(polygon.vertices.size(), 4u);
			const auto edgeA = polygon.vertices[1].position - polygon.vertices[0].position;
			const auto edgeB = polygon.vertices[2].position - polygon.vertices[0].position;
			EXPECT_GT(edgeA.cross(edgeB).dot(polygon.geometricNormal), 0.0f);
			for (const auto &vertex : polygon.vertices)
			{
				ASSERT_TRUE(vertex.normal.has_value());
				expectVectorNear(*vertex.normal, polygon.geometricNormal);
			}
			const auto &original = definition.shape().polygons().front();
			for (std::size_t i = 0; i < original.vertices.size(); ++i)
			{
				const std::size_t dest = value >= 4 ? original.vertices.size() - 1 - i : i;
				expectVectorNear(polygon.vertices[dest].position, cell.transformPosition(original.vertices[i].position));
				EXPECT_EQ(polygon.vertices[dest].uv, original.vertices[i].uv);
			}
			EXPECT_EQ(polygon.shell, Shape::Shell::Outer);
			ASSERT_TRUE(polygon.boundary.has_value());
		}
	}

	TEST(VoxelCellTest, FlippedTopAndBottomOuterFacesSwap)
	{
		Shape::Builder builder;
		builder.addSlot("top");
		builder.addPolygon("top", {vertex(0, 1, 0), vertex(0, 1, 1), vertex(1, 1, 1), vertex(1, 1, 0)});
		const Cell cell(Definition(builder.build(), {{"top", "roof"}}), Cell::Orientation::FlippedY0);
		const auto polygon = cell.orientedPolygons().front();
		EXPECT_EQ(polygon.boundary, Shape::Face::NegativeY);
		EXPECT_FLOAT_EQ(polygon.vertices[0].position.y, 0.0f);
	}

	TEST(VoxelShapeTest, OverlappingAndOppositeWindingPolygonsRemainDistinct)
	{
		Shape::Builder builder;
		builder.addSlot("sheet");
		const auto vertices = square();
		builder.addPolygon("sheet", vertices);
		builder.addPolygon("sheet", vertices);
		auto reversed = vertices;
		std::reverse(reversed.begin(), reversed.end());
		builder.addPolygon("sheet", reversed);
		const Shape shape = builder.build();
		ASSERT_EQ(shape.polygons().size(), 3u);
		EXPECT_EQ(shape.polygons()[0].geometricNormal, shape.polygons()[1].geometricNormal);
		EXPECT_EQ(shape.polygons()[0].geometricNormal, -shape.polygons()[2].geometricNormal);
	}

	TEST(VoxelCellTest, SixSidedCubeKeepsEachPhysicalFrontFaceUnderEveryOrientation)
	{
		const std::array<spk::Vector3, 8> corners = {
			spk::Vector3{0, 0, 0},
			{1, 0, 0},
			{1, 1, 0},
			{0, 1, 0},
			{0, 0, 1},
			{1, 0, 1},
			{1, 1, 1},
			{0, 1, 1}};
		const std::array<std::array<std::size_t, 4>, 6> faces = {
			std::array<std::size_t, 4>{0, 3, 2, 1},
			{4, 5, 6, 7},
			{0, 4, 7, 3},
			{1, 2, 6, 5},
			{0, 1, 5, 4},
			{3, 7, 6, 2}};
		Shape::Builder builder;
		builder.addSlot("cube");
		for (const auto &indices : faces)
		{
			std::vector<Shape::Vertex> vertices;
			for (const auto index : indices)
			{
				vertices.push_back({corners[index], {}, std::nullopt});
			}
			builder.addPolygon("cube", std::move(vertices));
		}
		const Definition definition(builder.build(), {{"cube", "stone"}});
		for (unsigned index = 0; index < 8; ++index)
		{
			const Cell cell(definition, static_cast<Cell::Orientation>(index));
			std::set<Shape::Face> boundaries;
			const auto polygons = cell.orientedPolygons();
			ASSERT_EQ(polygons.size(), 6u);
			for (const auto &polygon : polygons)
			{
				ASSERT_TRUE(polygon.boundary.has_value());
				boundaries.insert(*polygon.boundary);
				EXPECT_EQ(polygon.shell, Shape::Shell::Outer);
				const auto edgeA = polygon.vertices[1].position - polygon.vertices[0].position;
				const auto edgeB = polygon.vertices[2].position - polygon.vertices[0].position;
				EXPECT_GT(edgeA.cross(edgeB).dot(polygon.geometricNormal), 0.0f);
			}
			EXPECT_EQ(boundaries.size(), 6u);
		}
	}
}
