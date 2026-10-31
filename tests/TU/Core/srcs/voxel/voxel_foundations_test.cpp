#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <string>
#include <thread>
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
	static_assert(std::is_same_v<decltype(std::declval<const Shape::Instance &>().triangles()), const std::vector<Shape::Polygon> &>);
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
		builder.addSlot("glass");
		builder.addPolygon("wall", square());
		builder.addPolygon("glass", {vertex(0.2f, 0.1f, 0.2f), vertex(0.8f, 0.1f, 0.2f), vertex(0.5f, 0.7f, 0.2f)});
		const Shape shape = builder.build();
		const Definition definition(shape, {{"wall", {"wall"}}, {"glass", {"glass", 0.25f}}});
		ASSERT_EQ(shape.instance().triangles().size(), 3u);
		EXPECT_FLOAT_EQ(definition.opacity("wall"), 1.0f);
		EXPECT_FLOAT_EQ(definition.opacity("glass"), 0.25f);
		EXPECT_EQ(shape.instance().triangles()[0].shell, Shape::Shell::Outer);
		EXPECT_EQ(shape.instance().triangles()[0].boundary, Shape::Face::NegativeX);
		EXPECT_EQ(shape.instance().triangles()[2].shell, Shape::Shell::Inner);
		EXPECT_FALSE(shape.instance().triangles()[2].boundary.has_value());
		EXPECT_EQ(shape.instance().triangles()[0].geometricNormal, spk::Vector3(-1.0f, 0.0f, 0.0f));
		EXPECT_EQ(shape.instance().triangles()[0].vertices[2].normal.value(), spk::Vector3(-1.0f, 0.0f, 0.0f));
	}

	TEST(VoxelShapeTest, BuilderValidationIsAtomicForInvalidSlotsAndPolygons)
	{
		Shape::Builder builder;
		EXPECT_THROW(builder.addSlot(""), spk::Exception);
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
		EXPECT_TRUE(shape.instance().triangles().empty());
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
		EXPECT_EQ(shape.instance().triangles()[0].vertices[0].uv, spk::Vector2(-1.0f, 2.0f));
		(void)shape.instance().triangles();
		EXPECT_EQ(warnings, 1);
	}

	TEST(VoxelDefinitionTest, CopiesShareIdentityButNewBuildsDoNot)
	{
		const Shape shape = makeSquare();
		const Shape identical = makeSquare();
		EXPECT_FALSE(shape == identical);
		const Definition first(shape, {{"wall", {"stone"}}});
		const Definition copied = first;
		const Definition second(shape, {{"wall", {"glass"}}});
		const Definition third(identical, {{"wall", {"stone"}}});
		EXPECT_TRUE(first == copied);
		EXPECT_FALSE(first == second);
		EXPECT_FALSE(first == third);
		EXPECT_TRUE(first.shape() == second.shape());
		EXPECT_EQ(first.slot("wall").name, "stone");
		EXPECT_EQ(second.semanticSlot("wall"), "glass");
		EXPECT_EQ(first.shape().instance().triangles().size(), 2u);
		EXPECT_THROW(first.semanticSlot("none"), spk::Exception);
	}

	TEST(VoxelDefinitionTest, SlotMappingsMustBeCompleteAndValid)
	{
		const Shape shape = makeSquare();
		EXPECT_THROW((Definition(shape, {})), spk::Exception);
		EXPECT_THROW((Definition(shape, {{"missing", {"x"}}})), spk::Exception);
		EXPECT_THROW((Definition(shape, {{"wall", {""}}})), spk::Exception);
		EXPECT_NO_THROW((Definition(shape, {{"wall", {"stone"}}})));
	}

	TEST(VoxelDefinitionTest, OpacityIsValidatedAndDefaultsToOpaque)
	{
		const Shape shape = makeSquare();
		const Definition::SlotMapping slots{{"wall", {"stone"}}};
		EXPECT_FLOAT_EQ(Definition(shape, slots).opacity("wall"), 1.0f);
		EXPECT_FLOAT_EQ((Definition(shape, {{"wall", {"stone", 0.25f}}})).opacity("wall"), 0.25f);
		EXPECT_THROW((Definition(shape, {{"wall", {"stone", -0.1f}}})), spk::Exception);
		EXPECT_THROW((Definition(shape, {{"wall", {"stone", 1.1f}}})), spk::Exception);
		EXPECT_THROW((Definition(shape, {{"wall", {"stone", std::numeric_limits<float>::quiet_NaN()}}})), spk::Exception);
		EXPECT_THROW((Definition(shape, {{"missing", {"stone", 0.5f}}})), spk::Exception);
	}

	TEST(VoxelCellTest, DefaultAirIsValidAndDoesNotOcclude)
	{
		const Cell air;
		const Cell explicitAir(Definition::air(), Cell::HorizontalOrientation::ZPositive, Cell::VerticalOrientation::YNegative);
		EXPECT_TRUE(air.isAir());
		EXPECT_TRUE(explicitAir.isAir());
		EXPECT_TRUE(air.definition() == Definition::air());
		EXPECT_TRUE(air.definition().shape().instance().triangles().empty());
		EXPECT_EQ(air.horizontalOrientation(), Cell::HorizontalOrientation::XPositive);
		EXPECT_EQ(air.verticalOrientation(), Cell::VerticalOrientation::YPositive);
	}

	TEST(VoxelCellTest, AllEightOrientationPairsRetainDefinition)
	{
		const Definition definition(makeSquare(), {{"wall", {"stone"}}});
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			const Cell cell = makeOrientedCell(definition, index);
			EXPECT_TRUE(cell.definition() == definition);
			EXPECT_EQ(cell.horizontalOrientation(), Orientations[index].horizontal);
			EXPECT_EQ(cell.verticalOrientation(), Orientations[index].vertical);
			EXPECT_EQ(cell.definition().shape().instance().triangles().size(), 2u);
		}
		EXPECT_EQ(static_cast<unsigned>(Cell::HorizontalOrientation::XPositive), 0u);
		EXPECT_EQ(static_cast<unsigned>(Cell::HorizontalOrientation::ZNegative), 90u);
		EXPECT_EQ(static_cast<unsigned>(Cell::HorizontalOrientation::XNegative), 180u);
		EXPECT_EQ(static_cast<unsigned>(Cell::HorizontalOrientation::ZPositive), 270u);
	}

	TEST(VoxelShapeTest, OrientedInstancesExposePrecomputedTrianglesWithOriginalMetadata)
	{
		const Shape shape = makeSquare();
		const Shape copy = shape;
		const auto source = square();
		for (const auto &entry : Orientations)
		{
			const Shape::Orientation orientation{entry.horizontal, entry.vertical};
			const auto &triangles = shape.instance(orientation).triangles();
			ASSERT_EQ(triangles.size(), 2u);
			EXPECT_EQ(&triangles, &copy.instance(orientation).triangles());
			EXPECT_EQ(triangles[0].vertices[0].position, triangles[1].vertices[0].position);
			EXPECT_EQ(triangles[0].vertices[2].position, triangles[1].vertices[1].position);
			float totalArea = 0.0f;
			for (const auto &triangle : triangles)
			{
				ASSERT_EQ(triangle.vertices.size(), 3u);
				EXPECT_EQ(triangle.slotKey, "wall");
				EXPECT_EQ(triangle.shell, Shape::Shell::Outer);
				EXPECT_EQ(triangle.boundary, triangles[0].boundary);
				const auto a = triangle.vertices[1].position - triangle.vertices[0].position;
				const auto b = triangle.vertices[2].position - triangle.vertices[0].position;
				EXPECT_GT(a.cross(b).dot(triangle.geometricNormal), 0.0f);
				totalArea += static_cast<float>(a.cross(b).length()) * 0.5f;
				for (const auto &vertex : triangle.vertices)
				{
					EXPECT_TRUE(std::any_of(source.begin(), source.end(), [&](const Shape::Vertex &original) {
						return original.uv == vertex.uv;
					}));
					EXPECT_EQ(vertex.normal, triangle.geometricNormal);
				}
			}
			EXPECT_NEAR(totalArea, 1.0f, 1.0e-5f);
		}
	}

	TEST(VoxelShapeTest, ConvexPentagonProducesThreePrecomputedTriangles)
	{
		Shape::Builder builder;
		builder.addSlot("pentagon");
		const std::vector<Shape::Vertex> vertices = {
			vertex(0.0f, 0.0f, 0.0f),
			vertex(1.0f, 0.0f, 0.0f),
			vertex(1.0f, 0.5f, 0.0f),
			vertex(0.5f, 1.0f, 0.0f),
			vertex(0.0f, 0.5f, 0.0f)};
		builder.addPolygon("pentagon", vertices);
		const Shape shape = builder.build();
		for (const auto &entry : Orientations)
		{
			const Shape::Orientation orientation{entry.horizontal, entry.vertical};
			const auto &triangles = shape.instance(orientation).triangles();
			ASSERT_EQ(triangles.size(), 3u);
			for (const auto &triangle : triangles)
			{
				EXPECT_EQ(triangle.vertices.size(), 3u);
				EXPECT_EQ(triangle.slotKey, "pentagon");
				EXPECT_EQ(triangle.boundary, triangles[0].boundary);
				EXPECT_EQ(triangle.geometricNormal, triangles[0].geometricNormal);
			}
		}
	}

	TEST(VoxelShapeTest, EightOrientationsProvideStableInstancesAndPhysicalFrontFaces)
	{
		const Shape shape = makeSquare();
		const Shape copied = shape;
		const Shape independent = makeSquare();
		const std::array<Shape::Face, 8> expectedFaces = {
			Shape::Face::NegativeX,
			Shape::Face::PositiveZ,
			Shape::Face::PositiveX,
			Shape::Face::NegativeZ,
			Shape::Face::NegativeX,
			Shape::Face::PositiveZ,
			Shape::Face::PositiveX,
			Shape::Face::NegativeZ};
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			const auto orientation = Shape::Orientation{Orientations[index].horizontal, Orientations[index].vertical};
			const auto &instance = shape.instance(orientation);
			EXPECT_EQ(&instance, &copied.instance(orientation));
			EXPECT_NE(&instance, &independent.instance(orientation));
			ASSERT_EQ(instance.triangles().size(), 2u);
			const auto &polygon = instance.triangles()[0];
			EXPECT_EQ(polygon.boundary, expectedFaces[index]);
			EXPECT_EQ(polygon.shell, Shape::Shell::Outer);
			EXPECT_EQ(polygon.slotKey, "wall");
			ASSERT_EQ(polygon.vertices.size(), 3u);
			const auto edgeA = polygon.vertices[1].position - polygon.vertices[0].position;
			const auto edgeB = polygon.vertices[2].position - polygon.vertices[0].position;
			EXPECT_GT(edgeA.cross(edgeB).dot(polygon.geometricNormal), 0.0f);
			for (const auto &vertex : polygon.vertices)
			{
				ASSERT_TRUE(vertex.normal.has_value());
				EXPECT_EQ(*vertex.normal, polygon.geometricNormal);
				EXPECT_GE(vertex.position.x, 0.0f);
				EXPECT_LE(vertex.position.x, 1.0f);
				EXPECT_GE(vertex.position.y, 0.0f);
				EXPECT_LE(vertex.position.y, 1.0f);
				EXPECT_GE(vertex.position.z, 0.0f);
				EXPECT_LE(vertex.position.z, 1.0f);
			}
		}
	}

	TEST(VoxelShapeTest, VerticalFlipPreservesVertexUVAndSwapsTopBottomBoundaries)
	{
		Shape::Builder builder;
		builder.addSlot("face");
		const std::vector<Shape::Vertex> top{
			vertex(0, 1, 0, 0, 0),
			vertex(1, 1, 0, 1, 0),
			vertex(0, 1, 1, 0, 1)};
		const std::vector<Shape::Vertex> bottom{
			vertex(0, 0, 0, 0, 0),
			vertex(0, 0, 1, 0, 1),
			vertex(1, 0, 0, 1, 0)};
		builder.addPolygon("face", top);
		builder.addPolygon("face", bottom);
		const Shape shape = builder.build();
		const auto &original = shape.instance().triangles();
		const Shape::Orientation flippedOrientation{Shape::HorizontalOrientation::XPositive, Shape::VerticalOrientation::YNegative};
		const auto &flipped = shape.instance(flippedOrientation).triangles();
		ASSERT_EQ(flipped.size(), 2u);
		EXPECT_EQ(original[0].boundary, Shape::Face::PositiveY);
		EXPECT_EQ(original[1].boundary, Shape::Face::NegativeY);
		EXPECT_EQ(flipped[0].boundary, Shape::Face::NegativeY);
		EXPECT_EQ(flipped[1].boundary, Shape::Face::PositiveY);
		for (std::size_t polygonIndex = 0; polygonIndex < original.size(); ++polygonIndex)
		{
			const auto &source = original[polygonIndex];
			const auto &result = flipped[polygonIndex];
			EXPECT_EQ(result.geometricNormal.y, -source.geometricNormal.y);
			for (std::size_t index = 0; index < source.vertices.size(); ++index)
			{
				const auto &before = source.vertices[source.vertices.size() - 1 - index];
				const auto &after = result.vertices[index];
				EXPECT_EQ(after.uv, before.uv);
				EXPECT_EQ(after.position.y, 1.0f - before.position.y);
				EXPECT_EQ(after.position.x, before.position.x);
				EXPECT_EQ(after.position.z, before.position.z);
			}
			const auto a = result.vertices[1].position - result.vertices[0].position;
			const auto b = result.vertices[2].position - result.vertices[0].position;
			EXPECT_GT(a.cross(b).dot(result.geometricNormal), 0.0f);
		}
	}

	TEST(VoxelShapeTest, EightCachedInstancesHaveDistinctStableAddresses)
	{
		const Shape shape = makeSquare();
		const Shape copy = shape;
		const Shape independent = makeSquare();
		std::array<const Shape::Instance *, 8> addresses{};
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			const Shape::Orientation orientation{Orientations[index].horizontal, Orientations[index].vertical};
			const auto &instance = shape.instance(orientation);
			addresses[index] = &instance;
			EXPECT_EQ(&shape.instance(orientation), addresses[index]);
			EXPECT_EQ(&copy.instance(orientation), addresses[index]);
			EXPECT_NE(&independent.instance(orientation), addresses[index]);
			for (std::size_t previous = 0; previous < index; ++previous)
			{
				EXPECT_NE(addresses[index], addresses[previous]);
			}
		}
		EXPECT_EQ(&shape.instance(), addresses[0]);
	}

	TEST(VoxelShapeTest, SharedShapeCacheDoesNotDependOnDefinitionSemanticSlots)
	{
		const Shape shape = makeSquare();
		const Definition stone(shape, {{"wall", {"stone"}}});
		const Definition glass(shape, {{"wall", {"glass"}}});
		EXPECT_FALSE(stone == glass);
		EXPECT_TRUE(stone.shape() == glass.shape());
		for (const auto &pair : Orientations)
		{
			const Cell first(stone, pair.horizontal, pair.vertical);
			const Cell second(glass, pair.horizontal, pair.vertical);
			const Shape::Orientation orientation{pair.horizontal, pair.vertical};
			const auto &firstInstance = first.definition().shape().instance(orientation);
			const auto &secondInstance = second.definition().shape().instance(orientation);
			EXPECT_EQ(&firstInstance, &secondInstance);
			ASSERT_EQ(firstInstance.triangles().size(), 2u);
			EXPECT_EQ(firstInstance.triangles()[0].slotKey, "wall");
			EXPECT_EQ(first.definition().semanticSlot("wall"), "stone");
			EXPECT_EQ(second.definition().semanticSlot("wall"), "glass");
		}
	}

	TEST(VoxelShapeTest, FailedBuilderAdditionDoesNotInvalidatePublishedInstances)
	{
		Shape::Builder builder;
		builder.addSlot("wall");
		builder.addPolygon("wall", square());
		const Shape published = builder.build();
		std::array<const Shape::Instance *, 8> addresses{};
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			const auto &pair = Orientations[index];
			addresses[index] = &published.instance({pair.horizontal, pair.vertical});
		}
		EXPECT_THROW(builder.addPolygon("missing", square()), spk::Exception);
		const Shape next = builder.build();
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			const auto &pair = Orientations[index];
			const Shape::Orientation orientation{pair.horizontal, pair.vertical};
			const auto &cached = published.instance(orientation);
			EXPECT_EQ(&cached, addresses[index]);
			EXPECT_NE(&next.instance(orientation), addresses[index]);
			ASSERT_EQ(cached.triangles().size(), 2u);
			EXPECT_EQ(cached.triangles()[0].slotKey, "wall");
			EXPECT_EQ(next.instance(orientation).triangles().size(), 2u);
		}
	}

	TEST(VoxelShapeTest, CachedInstancesStayAliveAfterOriginalShapeIsDestroyed)
	{
		Shape survivor;
		const Shape::Instance *stored = nullptr;
		const Shape::Orientation orientation{
			Shape::HorizontalOrientation::ZNegative, Shape::VerticalOrientation::YNegative};
		{
			const Shape original = makeSquare();
			survivor = original;
			stored = &original.instance(orientation);
			EXPECT_EQ(stored, &survivor.instance(orientation));
		}
		EXPECT_EQ(stored, &survivor.instance(orientation));
		ASSERT_EQ(stored->triangles().size(), 2u);
		EXPECT_EQ(stored->triangles()[0].slotKey, "wall");
		EXPECT_EQ(stored->triangles()[0].boundary, Shape::Face::PositiveZ);
	}

	TEST(VoxelShapeTest, CachedInstancesSupportConcurrentReaders)
	{
		const Shape shape = makeSquare();
		const Shape copied = shape;
		std::array<const Shape::Instance *, 8> expected{};
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			const Shape::Orientation orientation{Orientations[index].horizontal, Orientations[index].vertical};
			expected[index] = &shape.instance(orientation);
		}

		std::atomic<bool> consistent{true};
		std::vector<std::thread> readers;
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			readers.emplace_back([&, index]() {
				const Shape::Orientation orientation{Orientations[index].horizontal, Orientations[index].vertical};
				for (unsigned iteration = 0; iteration < 512; ++iteration)
				{
					const auto &instance = (iteration % 2 == 0 ? shape : copied).instance(orientation);
					if (&instance != expected[index] || instance.triangles().size() != 2u ||
						instance.triangles()[0].slotKey != "wall")
					{
						consistent.store(false, std::memory_order_relaxed);
						break;
					}
				}
			});
		}
		for (auto &reader : readers)
		{
			reader.join();
		}
		EXPECT_TRUE(consistent.load(std::memory_order_relaxed));
	}

	TEST(VoxelShapeTest, EveryOrientationTransformsPositionsAndCustomLightingNormals)
	{
		Shape::Builder builder;
		builder.addSlot("panel");
		auto vertices = std::vector<Shape::Vertex>{
			vertex(0.2f, 0.3f, 0.4f, 0.15f, 0.85f),
			vertex(0.2f, 0.3f, 0.8f, 0.4f, 0.2f),
			vertex(0.2f, 0.7f, 0.8f, 0.9f, 0.6f)};
		vertices[0].normal = spk::Vector3{1.0f, 2.0f, 3.0f};
		builder.addPolygon("panel", vertices);
		const Shape shape = builder.build();

		const std::array<spk::Vector3, 8> expectedPositions = {
			spk::Vector3{0.2f, 0.3f, 0.4f},
			{0.4f, 0.3f, 0.8f},
			{0.8f, 0.3f, 0.6f},
			{0.6f, 0.3f, 0.2f},
			{0.2f, 0.7f, 0.4f},
			{0.4f, 0.7f, 0.8f},
			{0.8f, 0.7f, 0.6f},
			{0.6f, 0.7f, 0.2f}};
		const std::array<spk::Vector3, 8> expectedNormals = {
			spk::Vector3{1.0f, 2.0f, 3.0f},
			{3.0f, 2.0f, -1.0f},
			{-1.0f, 2.0f, -3.0f},
			{-3.0f, 2.0f, 1.0f},
			{1.0f, -2.0f, 3.0f},
			{3.0f, -2.0f, -1.0f},
			{-1.0f, -2.0f, -3.0f},
			{-3.0f, -2.0f, 1.0f}};
		const std::array<spk::Vector3, 4> expectedGeometricNormals = {
			spk::Vector3{-1.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 1.0f},
			{1.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, -1.0f}};

		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			SCOPED_TRACE(index);
			const Shape::Orientation orientation{Orientations[index].horizontal, Orientations[index].vertical};
			const auto &polygons = shape.instance(orientation).triangles();
			ASSERT_EQ(polygons.size(), 1u);
			const auto &polygon = polygons[0];
			ASSERT_EQ(polygon.vertices.size(), 3u);
			const std::size_t anchorIndex = index >= 4 ? 2u : 0u;
			const auto &anchor = polygon.vertices[anchorIndex];
			EXPECT_NEAR((anchor.position - expectedPositions[index]).length(), 0.0, 1.0e-6);
			EXPECT_EQ(anchor.uv, spk::Vector2(0.15f, 0.85f));
			ASSERT_TRUE(anchor.normal.has_value());
			EXPECT_EQ(*anchor.normal, expectedNormals[index]);
			EXPECT_EQ(polygon.geometricNormal, expectedGeometricNormals[index % 4]);
			EXPECT_EQ(polygon.slotKey, "panel");
			EXPECT_EQ(polygon.shell, Shape::Shell::Inner);
			EXPECT_FALSE(polygon.boundary.has_value());
			const auto edgeA = polygon.vertices[1].position - polygon.vertices[0].position;
			const auto edgeB = polygon.vertices[2].position - polygon.vertices[0].position;
			EXPECT_GT(edgeA.cross(edgeB).dot(polygon.geometricNormal), 0.0f);
		}
	}

	TEST(VoxelShapeTest, PublishedInstancesRemainUnchangedWhenBuilderIsReused)
	{
		Shape::Builder builder;
		builder.addSlot("first");
		builder.addPolygon("first", square());
		const Shape original = builder.build();
		const Shape originalCopy = original;
		std::array<const Shape::Instance *, 8> before{};
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			const Shape::Orientation orientation{Orientations[index].horizontal, Orientations[index].vertical};
			before[index] = &original.instance(orientation);
		}

		builder.addSlot("second");
		builder.addPolygon("second", square());
		const Shape extended = builder.build();
		const Shape repeated = builder.build();
		EXPECT_FALSE(original == extended);
		EXPECT_FALSE(extended == repeated);
		EXPECT_EQ(original.slots().size(), 1u);
		EXPECT_EQ(extended.slots().size(), 2u);
		EXPECT_FLOAT_EQ((Definition(original, {{"first", {"first", 0.5f}}})).opacity("first"), 0.5f);
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			const Shape::Orientation orientation{Orientations[index].horizontal, Orientations[index].vertical};
			const auto &instance = original.instance(orientation);
			EXPECT_EQ(&instance, before[index]);
			EXPECT_EQ(&originalCopy.instance(orientation), before[index]);
			ASSERT_EQ(instance.triangles().size(), 2u);
			EXPECT_EQ(instance.triangles()[0].slotKey, "first");
			EXPECT_EQ(extended.instance(orientation).triangles().size(), 4u);
			EXPECT_EQ(repeated.instance(orientation).triangles().size(), 4u);
		}
	}

	TEST(VoxelShapeTest, EmptyShapePublishesEightReusableEmptyInstances)
	{
		const Shape empty = Shape::Builder{}.build();
		const Shape copied = empty;
		std::array<const Shape::Instance *, 8> addresses{};
		for (std::size_t index = 0; index < Orientations.size(); ++index)
		{
			const Shape::Orientation orientation{Orientations[index].horizontal, Orientations[index].vertical};
			const auto &instance = empty.instance(orientation);
			addresses[index] = &instance;
			EXPECT_TRUE(instance.empty());
			EXPECT_TRUE(instance.triangles().empty());
			EXPECT_EQ(addresses[index], &copied.instance(orientation));
			for (std::size_t previous = 0; previous < index; ++previous)
			{
				EXPECT_NE(addresses[index], addresses[previous]);
			}
		}
	}

	TEST(VoxelShapeTest, InvalidOrientationsThrowInsteadOfIndexingOutsideInstances)
	{
		const Shape shape = makeSquare();
		const Shape::Orientation invalidHorizontal{static_cast<Shape::HorizontalOrientation>(45), Shape::VerticalOrientation::YPositive};
		const Shape::Orientation invalidVertical{Shape::HorizontalOrientation::XPositive, static_cast<Shape::VerticalOrientation>(2)};
		EXPECT_THROW((void)shape.instance(invalidHorizontal), spk::Exception);
		EXPECT_THROW((void)shape.instance(invalidVertical), spk::Exception);
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
		ASSERT_EQ(shape.instance().triangles().size(), 6u);
		ASSERT_EQ(shape.slots().size(), 1u);
		const Definition definition(shape, {{"sheet", {"stone"}}});
		for (const auto &polygon : shape.instance().triangles())
		{
			EXPECT_EQ(polygon.slotKey, "sheet");
			EXPECT_EQ(definition.semanticSlot(polygon.slotKey), "stone");
		}
		EXPECT_EQ(shape.instance().triangles()[0].geometricNormal, shape.instance().triangles()[2].geometricNormal);
		EXPECT_EQ(shape.instance().triangles()[0].geometricNormal, -shape.instance().triangles()[4].geometricNormal);
	}

}
