#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
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

	struct OrientationPair
	{
		Cell::HorizontalOrientation horizontal;
		Cell::VerticalOrientation vertical;
	};

	const std::array<OrientationPair, 8> Orientations = {
		OrientationPair{Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YPositive},
		{Cell::HorizontalOrientation::ZNegative, Cell::VerticalOrientation::YPositive},
		{Cell::HorizontalOrientation::XNegative, Cell::VerticalOrientation::YPositive},
		{Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YPositive},
		{Cell::HorizontalOrientation::XPositive, Cell::VerticalOrientation::YNegative},
		{Cell::HorizontalOrientation::ZNegative, Cell::VerticalOrientation::YNegative},
		{Cell::HorizontalOrientation::XNegative, Cell::VerticalOrientation::YNegative},
		{Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YNegative}};

	Cell makeOrientedCell(const Definition &definition, std::size_t index)
	{
		return Cell(definition, Orientations[index].horizontal, Orientations[index].vertical);
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
		EXPECT_FALSE(first == second);
		EXPECT_TRUE(first == Shape(first));
		const Shape another = firstBuilder.build();
		EXPECT_FALSE(first == another);
		firstBuilder.addSlot("later");
		const Shape extended = firstBuilder.build();
		EXPECT_EQ(extended.slots().size(), 1u);
		EXPECT_TRUE(first.slots().empty());
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
		EXPECT_FALSE(shape == identical);
		const Definition first(shape, {{"wall", "stone"}});
		const Definition copied = first;
		const Definition second(shape, {{"wall", "glass"}});
		const Definition third(identical, {{"wall", "stone"}});
		EXPECT_TRUE(first == copied);
		EXPECT_FALSE(first == second);
		EXPECT_FALSE(first == third);
		EXPECT_TRUE(first.shape() == second.shape());
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
		const Cell explicitAir(Definition::air(), Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YNegative);
		EXPECT_TRUE(air.isAir());
		EXPECT_TRUE(explicitAir.isAir());
		EXPECT_TRUE(air.definition() == Definition::air());
		EXPECT_TRUE(air.definition().shape().polygons().empty());
		EXPECT_EQ(air.horizontalOrientation(), Cell::HorizontalOrientation::XPositive);
		EXPECT_EQ(air.verticalOrientation(), Cell::VerticalOrientation::YPositive);
	}

	TEST(VoxelCellTest, AllEightOrientationPairsRetainDefinition)
	{
		const Definition definition(makeSquare(), {{"wall", "stone"}});
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			const Cell cell = makeOrientedCell(definition, index);
			EXPECT_TRUE(cell.definition() == definition);
			EXPECT_EQ(cell.horizontalOrientation(), Orientations[index].horizontal);
			EXPECT_EQ(cell.verticalOrientation(), Orientations[index].vertical);
			EXPECT_EQ(cell.definition().shape().polygons().size(), 1u);
		}
		EXPECT_EQ(static_cast<unsigned>(Cell::HorizontalOrientation::XPositive), 0u);
		EXPECT_EQ(static_cast<unsigned>(Cell::HorizontalOrientation::ZNegative), 90u);
		EXPECT_EQ(static_cast<unsigned>(Cell::HorizontalOrientation::XNegative), 180u);
		EXPECT_EQ(static_cast<unsigned>(Cell::HorizontalOrientation::ZPositive), 270u);
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
		ASSERT_EQ(shape.slots().size(), 1u);
		const Definition definition(shape, {{"sheet", "stone"}});
		for (const auto &polygon : shape.polygons())
		{
			EXPECT_EQ(polygon.slotKey, "sheet");
			EXPECT_EQ(definition.semanticSlot(polygon.slotKey), "stone");
		}
		EXPECT_EQ(shape.polygons()[0].geometricNormal, shape.polygons()[1].geometricNormal);
		EXPECT_EQ(shape.polygons()[0].geometricNormal, -shape.polygons()[2].geometricNormal);
	}

}
