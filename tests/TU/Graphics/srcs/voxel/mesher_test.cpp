#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <future>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "exception.hpp"
#include "voxel/mesher.hpp"

namespace
{
	using namespace spk::Voxel;
	using Face = Shape::Face;
	using Position = spk::Vector3UInt;
	using Horizontal = Shape::HorizontalOrientation;
	using Vertical = Shape::VerticalOrientation;

	constexpr std::array<const char *, 4> Names{"cube", "slab", "slope", "stair"};
	constexpr std::array<Face, 6> Faces{Face::NegativeX, Face::PositiveX, Face::NegativeY, Face::PositiveY, Face::NegativeZ, Face::PositiveZ};
	constexpr std::array<Horizontal, 4> Horizontals{Horizontal::XPositive, Horizontal::ZPositive, Horizontal::XNegative, Horizontal::ZNegative};
	constexpr std::array<Vertical, 2> Verticals{Vertical::YPositive, Vertical::YNegative};

	[[nodiscard]] constexpr std::array<Shape::Orientation, 8> orientations()
	{
		std::array<Shape::Orientation, 8> result{};
		for (std::size_t i = 0; i < result.size(); ++i)
		{
			result[i] = {Horizontals[i % Horizontals.size()], Verticals[i / Horizontals.size()]};
		}
		return result;
	}

	constexpr auto Orientations = orientations();

	[[nodiscard]] Shape fixture(const std::string &name)
	{
		Shape::Builder builder;
		builder.addFromJSON(spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (name + ".json")));
		return builder.build();
	}

	[[nodiscard]] Definition definition(const Shape &shape, const std::string &prefix, float opacity = 1.0f)
	{
		Definition::SlotMapping mapping;
		for (const auto &slot : shape.slots())
		{
			mapping.emplace(slot.name, Definition::Slot{prefix + slot.name, opacity});
		}
		return Definition(shape, std::move(mapping));
	}

	[[nodiscard]] Definition uniformDefinition(const Shape &shape, const std::string &material, float opacity)
	{
		Definition::SlotMapping mapping;
		for (const auto &slot : shape.slots())
		{
			mapping.emplace(slot.name, Definition::Slot{material, opacity});
		}
		return Definition(shape, std::move(mapping));
	}

	[[nodiscard]] Definition::Registry registry(const std::vector<Definition> &definitions)
	{
		Definition::Registry::Builder builder;
		for (std::size_t i = 0; i < definitions.size(); ++i)
		{
			builder.add(static_cast<std::uint32_t>(i + 1), definitions[i]);
		}
		return builder.build();
	}

	[[nodiscard]] Volume single(const Definition &definition, Shape::Orientation orientation = {})
	{
		Volume::Builder builder({1, 1, 1});
		builder.bind(registry({definition}));
		builder.set({0, 0, 0}, 1, orientation.horizontal, orientation.vertical);
		return builder.build();
	}

	[[nodiscard]] Position neighbor(Face face)
	{
		Position result{1, 1, 1};
		auto &axis = result.data[static_cast<std::size_t>(face) / 2];
		axis = (static_cast<unsigned>(face) & 1U) ? 2 : 0;
		return result;
	}

	[[nodiscard]] Volume pair(
		const Definition &source,
		Shape::Orientation sourceOrientation,
		const Definition &blocker,
		Shape::Orientation blockerOrientation,
		Face face)
	{
		Volume::Builder builder({3, 3, 3});
		builder.bind(registry({source, blocker}));
		builder.set({1, 1, 1}, 1, sourceOrientation.horizontal, sourceOrientation.vertical);
		builder.set(neighbor(face), 2, blockerOrientation.horizontal, blockerOrientation.vertical);
		return builder.build();
	}

	[[nodiscard]] std::vector<Geometry::Vertex> vertices(const Geometry::SurfaceMesh &mesh)
	{
		const auto view = mesh.layout().vertexBuffer().cast<Geometry::Vertex>();
		return {view.begin(), view.end()};
	}

	[[nodiscard]] std::vector<std::uint32_t> indices(const Geometry::SurfaceMesh &mesh)
	{
		const auto view = mesh.layout().indexBuffer().cast<std::uint32_t>();
		return {view.begin(), view.end()};
	}

	void expectEqual(const Geometry &a, const Geometry &b)
	{
		ASSERT_EQ(a.materialSlots().size(), b.materialSlots().size());
		for (std::size_t i = 0; i < a.materialSlots().size(); ++i)
		{
			EXPECT_EQ(a.materialSlots()[i].name, b.materialSlots()[i].name);
			EXPECT_FLOAT_EQ(a.materialSlots()[i].opacity, b.materialSlots()[i].opacity);
		}
		auto compare = [](const Geometry::SurfaceMesh &left, const Geometry::SurfaceMesh &right) {
			EXPECT_EQ(indices(left), indices(right));
			const auto a = vertices(left);
			const auto b = vertices(right);
			ASSERT_EQ(a.size(), b.size());
			for (std::size_t i = 0; i < a.size(); ++i)
			{
				EXPECT_EQ(a[i].position, b[i].position);
				EXPECT_EQ(a[i].normal, b[i].normal);
				EXPECT_EQ(a[i].uv, b[i].uv);
				EXPECT_EQ(a[i].materialIndex, b[i].materialIndex);
			}
		};
		compare(a.opaqueMesh(), b.opaqueMesh());
		compare(a.transparentMesh(), b.transparentMesh());
		EXPECT_EQ(a.depthMesh().vertexCount(), b.depthMesh().vertexCount());
		EXPECT_EQ(a.depthMesh().indexCount(), b.depthMesh().indexCount());
	}

	[[nodiscard]] Face opposite(Face face)
	{
		return static_cast<Face>(static_cast<unsigned>(face) ^ 1U);
	}

	[[nodiscard]] spk::Vector3 offset(Face face)
	{
		switch (face)
		{
		case Face::NegativeX:
			return {-1, 0, 0};
		case Face::PositiveX:
			return {1, 0, 0};
		case Face::NegativeY:
			return {0, -1, 0};
		case Face::PositiveY:
			return {0, 1, 0};
		case Face::NegativeZ:
			return {0, 0, -1};
		case Face::PositiveZ:
			return {0, 0, 1};
		}
		return {};
	}

	[[nodiscard]] spk::Vector2 project(const spk::Vector3 &position, Face face)
	{
		if (face == Face::NegativeX || face == Face::PositiveX)
		{
			return {position.y, position.z};
		}
		if (face == Face::NegativeY || face == Face::PositiveY)
		{
			return {position.x, position.z};
		}
		return {position.x, position.y};
	}

	[[nodiscard]] bool inside(const Shape::Polygon &polygon, Face face, spk::Vector2 sample)
	{
		bool positive = false;
		bool negative = false;
		for (std::size_t i = 0; i < polygon.vertices.size(); ++i)
		{
			const auto a = project(polygon.vertices[i].position, face);
			const auto b = project(polygon.vertices[(i + 1) % polygon.vertices.size()].position, face);
			const float side = (b.x - a.x) * (sample.y - a.y) - (b.y - a.y) * (sample.x - a.x);
			positive |= side > 1.0e-6f;
			negative |= side < -1.0e-6f;
		}
		return !(positive && negative);
	}

	[[nodiscard]] float sampledVisibleArea(
		const Shape &source,
		Shape::Orientation sourceOrientation,
		const Shape &blocker,
		Shape::Orientation blockerOrientation,
		Face face)
	{
		constexpr int resolution = 32;
		float visible = 0;
		for (int y = 0; y < resolution; ++y)
		{
			for (int x = 0; x < resolution; ++x)
			{
				const spk::Vector2 sample{
					(static_cast<float>(x) + 0.37f) / resolution,
					(static_cast<float>(y) + 0.63f) / resolution};
				bool blocked = false;
				for (const auto &polygon : blocker.instance(blockerOrientation).triangles())
				{
					if (polygon.shell == Shape::Shell::Outer && polygon.boundary == opposite(face) &&
						polygon.geometricNormal.dot(offset(face)) < 0 && inside(polygon, face, sample))
					{
						blocked = true;
					}
				}
				if (blocked)
				{
					continue;
				}
				for (const auto &polygon : source.instance(sourceOrientation).triangles())
				{
					if (polygon.shell == Shape::Shell::Outer && polygon.boundary == face && inside(polygon, face, sample))
					{
						visible += 1.0f;
					}
				}
			}
		}
		return visible / (resolution * resolution);
	}

	[[nodiscard]] float generatedBoundaryArea(const Geometry &model, Face face)
	{
		const auto verticesView = model.opaqueMesh().layout().vertexBuffer().cast<Geometry::Vertex>();
		const auto indexView = model.opaqueMesh().layout().indexBuffer().cast<std::uint32_t>();
		const std::size_t axis = static_cast<std::size_t>(face) / 2;
		const float plane = (static_cast<unsigned>(face) & 1U) ? 2.0f : 1.0f;
		float area = 0;
		for (std::size_t i = 0; i < indexView.size(); i += 3)
		{
			const auto &a = verticesView[indexView[i]];
			const auto &b = verticesView[indexView[i + 1]];
			const auto &c = verticesView[indexView[i + 2]];
			if (!model.materialSlots()[a.materialIndex].name.starts_with("source_"))
			{
				continue;
			}
			if (std::abs(a.position.data[axis] - plane) > 1.0e-5f ||
				std::abs(b.position.data[axis] - plane) > 1.0e-5f ||
				std::abs(c.position.data[axis] - plane) > 1.0e-5f)
			{
				continue;
			}
			area += static_cast<float>((b.position - a.position).cross(c.position - a.position).length()) * 0.5f;
		}
		return area;
	}

	[[nodiscard]] float shadowArea(const Geometry &model);
	[[nodiscard]] float visualArea(const Geometry::SurfaceMesh &mesh);

	void expectShadowGeometryMatchesOpaque(const Geometry &model)
	{
		const auto visualVertices = model.opaqueMesh().layout().vertexBuffer().cast<Geometry::Vertex>();
		const auto visualIndices = model.opaqueMesh().layout().indexBuffer().cast<std::uint32_t>();
		const auto shadowVertices = model.depthMesh().layout().vertexBuffer().cast<spk::Vector3>();
		const auto shadowIndices = model.depthMesh().layout().indexBuffer().cast<std::uint32_t>();
		ASSERT_LE(shadowIndices.size(), visualIndices.size());
		ASSERT_EQ(shadowIndices.size() % 3, 0u);
		if (shadowIndices.size() != visualIndices.size())
		{
			EXPECT_NEAR(shadowArea(model), visualArea(model.opaqueMesh()), 1.0e-4f);
			for (const auto index : shadowIndices)
			{
				ASSERT_LT(index, shadowVertices.size());
			}
			return;
		}
		ASSERT_EQ(visualVertices.size(), shadowVertices.size());
		ASSERT_EQ(visualIndices.size(), shadowIndices.size());
		for (std::size_t i = 0; i < visualVertices.size(); ++i)
		{
			EXPECT_EQ(shadowVertices[i], visualVertices[i].position);
		}
		for (std::size_t i = 0; i < visualIndices.size(); ++i)
		{
			EXPECT_EQ(shadowIndices[i], visualIndices[i]);
		}
	}

	[[nodiscard]] float shadowArea(const Geometry &model)
	{
		const auto vertex = model.depthMesh().layout().vertexBuffer().cast<spk::Vector3>();
		const auto index = model.depthMesh().layout().indexBuffer().cast<std::uint32_t>();
		float result = 0.0f;
		for (std::size_t i = 0; i < index.size(); i += 3)
		{
			const auto &a = vertex[index[i]];
			const auto &b = vertex[index[i + 1]];
			const auto &c = vertex[index[i + 2]];
			result += static_cast<float>((b - a).cross(c - a).length()) * 0.5f;
		}
		return result;
	}

	[[nodiscard]] float visualArea(const Geometry::SurfaceMesh &mesh)
	{
		const auto vertex = mesh.layout().vertexBuffer().cast<Geometry::Vertex>();
		const auto index = mesh.layout().indexBuffer().cast<std::uint32_t>();
		float result = 0.0f;
		for (std::size_t i = 0; i < index.size(); i += 3)
		{
			const auto &a = vertex[index[i]].position;
			const auto &b = vertex[index[i + 1]].position;
			const auto &c = vertex[index[i + 2]].position;
			result += static_cast<float>((b - a).cross(c - a).length()) * 0.5f;
		}
		return result;
	}

	void expectValidTriangles(const Geometry &model, const Geometry::SurfaceMesh &mesh, float opacity)
	{
		const auto vertex = mesh.layout().vertexBuffer().cast<Geometry::Vertex>();
		const auto index = mesh.layout().indexBuffer().cast<std::uint32_t>();
		ASSERT_EQ(index.size() % 3, 0u);
		for (std::size_t i = 0; i < index.size(); i += 3)
		{
			ASSERT_LT(index[i], vertex.size());
			ASSERT_LT(index[i + 1], vertex.size());
			ASSERT_LT(index[i + 2], vertex.size());
			const auto &a = vertex[index[i]];
			const auto &b = vertex[index[i + 1]];
			const auto &c = vertex[index[i + 2]];
			EXPECT_GT((b.position - a.position).cross(c.position - a.position).length(), 0.0);
			EXPECT_EQ(a.materialIndex, b.materialIndex);
			EXPECT_EQ(a.materialIndex, c.materialIndex);
			ASSERT_LT(a.materialIndex, model.materialSlots().size());
			EXPECT_FLOAT_EQ(model.materialSlots()[a.materialIndex].opacity, opacity);
		}
	}

	void expectNoShadowTrianglesOnPlane(const Geometry &model, Face face)
	{
		const auto vertex = model.depthMesh().layout().vertexBuffer().cast<spk::Vector3>();
		const auto index = model.depthMesh().layout().indexBuffer().cast<std::uint32_t>();
		const std::size_t axis = static_cast<std::size_t>(face) / 2;
		const float plane = (static_cast<unsigned>(face) & 1U) ? 2.0f : 1.0f;
		for (std::size_t i = 0; i < index.size(); i += 3)
		{
			const auto &a = vertex[index[i]];
			const auto &b = vertex[index[i + 1]];
			const auto &c = vertex[index[i + 2]];
			const bool shared = a.data[axis] == plane && b.data[axis] == plane && c.data[axis] == plane;
			EXPECT_FALSE(shared);
		}
	}

	class ExternalMesher final : public Mesher
	{
	public:
		using State = OutsideCell::State;
		OutsideCell state{};
		std::optional<Face> onlyFace;

	protected:
		[[nodiscard]] OutsideCell outsideCell(Position, Face face) const override
		{
			if (onlyFace && face != *onlyFace)
			{
				return {};
			}
			return state;
		}
	};
}

TEST(MesherGeometry, StandaloneShapesProduceExactShadowSurfacesInEveryOrientation)
{
	constexpr std::array<float, 4> ExpectedAreas{6.0f, 4.0f, 3.0f + 1.41421356f, 5.5f};
	constexpr std::array<std::size_t, 4> ExpectedIndices{36u, 36u, 24u, 60u};
	for (std::size_t i = 0; i < Names.size(); ++i)
	{
		const Shape shape = fixture(Names[i]);
		for (const auto orientation : Orientations)
		{
			SCOPED_TRACE(::testing::Message() << Names[i] << " orientation " << static_cast<unsigned>(orientation.horizontal) << "/" << static_cast<unsigned>(orientation.vertical));
			const Geometry model = Mesher().bake(single(definition(shape, "stone_"), orientation));
			EXPECT_EQ(model.opaqueMesh().indexCount(), ExpectedIndices[i]);
			EXPECT_TRUE(model.transparentMesh().empty());
			EXPECT_EQ(model.depthMesh().indexCount(), ExpectedIndices[i]);
			EXPECT_NEAR(shadowArea(model), ExpectedAreas[i], 1.0e-4f);
			expectValidTriangles(model, model.opaqueMesh(), 1.0f);
			expectShadowGeometryMatchesOpaque(model);
		}
	}
}

TEST(MesherGeometry, IsolatedInteriorCellMatchesTranslatedStandaloneGeometry)
{
	const spk::Vector3 translation{1, 1, 1};
	for (const char *name : Names)
	{
		const Shape shape = fixture(name);
		for (const auto orientation : Orientations)
		{
			for (const float opacity : {1.0f, 0.5f})
			{
				SCOPED_TRACE(::testing::Message() << name << " orientation " << static_cast<unsigned>(orientation.horizontal) << "/" << static_cast<unsigned>(orientation.vertical) << " opacity " << opacity);
				const Definition material = uniformDefinition(shape, "isolated", opacity);
				Volume::Builder builder({3, 3, 3});
				builder.bind(registry({material}));
				builder.set({1, 1, 1}, 1, orientation.horizontal, orientation.vertical);
				const Geometry interior = Mesher().bake(builder.build());
				const Geometry standalone = Mesher().bake(single(material, orientation));
				ASSERT_EQ(interior.materialSlots().size(), standalone.materialSlots().size());
				for (std::size_t index = 0; index < interior.materialSlots().size(); ++index)
				{
					EXPECT_EQ(interior.materialSlots()[index].name, standalone.materialSlots()[index].name);
					EXPECT_FLOAT_EQ(interior.materialSlots()[index].opacity, standalone.materialSlots()[index].opacity);
				}
				auto compare = [&](const Geometry::SurfaceMesh &actual, const Geometry::SurfaceMesh &expected) {
					EXPECT_EQ(indices(actual), indices(expected));
					const auto a = vertices(actual);
					const auto b = vertices(expected);
					ASSERT_EQ(a.size(), b.size());
					for (std::size_t index = 0; index < a.size(); ++index)
					{
						EXPECT_EQ(a[index].position, b[index].position + translation);
						EXPECT_EQ(a[index].normal, b[index].normal);
						EXPECT_EQ(a[index].uv, b[index].uv);
						EXPECT_EQ(a[index].materialIndex, b[index].materialIndex);
					}
				};
				compare(interior.opaqueMesh(), standalone.opaqueMesh());
				compare(interior.transparentMesh(), standalone.transparentMesh());
				EXPECT_EQ(interior.depthMesh().indexCount(), standalone.depthMesh().indexCount());
				EXPECT_EQ(interior.depthMesh().vertexCount(), standalone.depthMesh().vertexCount());
				expectShadowGeometryMatchesOpaque(interior);
			}
		}
	}
}

TEST(MesherGeometry, AdjacentCubesHaveNoInternalShadowFacesInAnyDirection)
{
	const Shape cube = fixture("cube");
	for (const Face face : Faces)
	{
		SCOPED_TRACE(static_cast<unsigned>(face));
		const Geometry model = Mesher().bake(pair(definition(cube, "source_"), {}, definition(cube, "other_"), {}, face));
		EXPECT_EQ(model.opaqueMesh().indexCount(), 60u);
		EXPECT_EQ(model.depthMesh().indexCount(), 36u);
		EXPECT_NEAR(shadowArea(model), 10.0f, 1.0e-5f);
		expectNoShadowTrianglesOnPlane(model, face);
		expectValidTriangles(model, model.opaqueMesh(), 1.0f);
		expectShadowGeometryMatchesOpaque(model);
	}
}

TEST(MesherGeometry, SolidMixedMaterialChunkCollapsesToTwelveShadowTriangles)
{
	const auto cube = fixture("cube");
	Volume::Builder builder({16, 16, 16});
	builder.bind(registry({definition(cube, "stone_"), definition(cube, "grass_")}));
	for (std::uint32_t z = 0; z < 16; ++z)
	{
		for (std::uint32_t y = 0; y < 16; ++y)
		{
			for (std::uint32_t x = 0; x < 16; ++x)
			{
				builder.set({x, y, z}, 1 + (x + y + z) % 2);
			}
		}
	}
	const Geometry model = Mesher().bake(builder.build());
	EXPECT_EQ(model.opaqueMesh().indexCount(), 6u * 16u * 16u * 6u);
	EXPECT_EQ(model.depthMesh().indexCount(), 36u);
	EXPECT_NEAR(shadowArea(model), 1536.0f, 1.0e-4f);
	const auto &bounds = model.depthMesh().bounds();
	EXPECT_EQ(bounds.minimum, (spk::Vector3{0, 0, 0}));
	EXPECT_EQ(bounds.maximum, (spk::Vector3{16, 16, 16}));
	const auto positions = model.depthMesh().layout().vertexBuffer().cast<spk::Vector3>();
	const auto indices = model.depthMesh().layout().indexBuffer().cast<std::uint32_t>();
	for (std::size_t i = 0; i < indices.size(); i += 3)
	{
		const auto &a = positions[indices[i]];
		const auto &b = positions[indices[i + 1]];
		const auto &c = positions[indices[i + 2]];
		const auto normal = (b - a).cross(c - a);
		EXPECT_GT(normal.x * (a.x - 8) + normal.y * (a.y - 8) + normal.z * (a.z - 8), 0.0f);
	}
}

TEST(MesherGeometry, SimplifiedShadowSurfacePreservesHoleAndWinding)
{
	const auto stone = definition(fixture("cube"), "stone_");
	Volume::Builder builder({3, 1, 3});
	builder.bind(registry({stone}));
	for (std::uint32_t z = 0; z < 3; ++z)
	{
		for (std::uint32_t x = 0; x < 3; ++x)
		{
			if (x != 1 || z != 1)
			{
				builder.set({x, 0, z}, 1);
			}
		}
	}
	const Geometry model = Mesher().bake(builder.build());
	EXPECT_LT(model.depthMesh().indexCount(), model.opaqueMesh().indexCount());
	EXPECT_NEAR(shadowArea(model), 32.0f, 1.0e-5f);
	const auto positions = model.depthMesh().layout().vertexBuffer().cast<spk::Vector3>();
	const auto indices = model.depthMesh().layout().indexBuffer().cast<std::uint32_t>();
	for (std::uint32_t z = 0; z < 3; ++z)
	{
		for (std::uint32_t x = 0; x < 3; ++x)
		{
			const spk::Vector3 p{static_cast<float>(x) + 0.31f, 1, static_cast<float>(z) + 0.43f};
			bool covered = false;
			for (std::size_t i = 0; i < indices.size(); i += 3)
			{
				const auto &a = positions[indices[i]];
				const auto &b = positions[indices[i + 1]];
				const auto &c = positions[indices[i + 2]];
				if (a.y != 1 || b.y != 1 || c.y != 1)
				{
					continue;
				}
				EXPECT_GT((b - a).cross(c - a).y, 0.0f);
				if ((b - a).cross(p - a).y >= 0 && (c - b).cross(p - b).y >= 0 && (a - c).cross(p - c).y >= 0)
				{
					covered = true;
				}
			}
			EXPECT_EQ(covered, x != 1 || z != 1);
		}
	}
}

TEST(MesherGeometry, AdjacentHalfSlabsMergeWithoutChangingTheirHeight)
{
	const auto slab = definition(fixture("slab"), "stone_");
	Volume::Builder builder({4, 1, 3});
	builder.bind(registry({slab}));
	for (std::uint32_t z = 0; z < 3; ++z)
	{
		for (std::uint32_t x = 0; x < 4; ++x)
		{
			builder.set({x, 0, z}, 1);
		}
	}
	const Geometry model = Mesher().bake(builder.build());
	EXPECT_EQ(model.depthMesh().indexCount(), 36u);
	EXPECT_LT(model.depthMesh().indexCount(), model.opaqueMesh().indexCount());
	EXPECT_NEAR(shadowArea(model), 31.0f, 1.0e-5f);
	EXPECT_NEAR(shadowArea(model), visualArea(model.opaqueMesh()), 1.0e-5f);
	const auto &bounds = model.depthMesh().bounds();
	EXPECT_EQ(bounds.minimum, (spk::Vector3{0, 0, 0}));
	EXPECT_EQ(bounds.maximum, (spk::Vector3{4, 0.5f, 3}));
}

TEST(MesherGeometry, HalfSlabsOnDifferentPlanesKeepTheirSeparateShadowSurfaces)
{
	const auto slab = definition(fixture("slab"), "stone_");
	Volume::Builder builder({2, 1, 1});
	builder.bind(registry({slab}));
	builder.set({0, 0, 0}, 1, Horizontal::XPositive, Vertical::YPositive);
	builder.set({1, 0, 0}, 1, Horizontal::XPositive, Vertical::YNegative);
	const Geometry model = Mesher().bake(builder.build());
	EXPECT_NEAR(shadowArea(model), 8.0f, 1.0e-5f);
	EXPECT_NEAR(shadowArea(model), visualArea(model.opaqueMesh()), 1.0e-5f);
	const auto &bounds = model.depthMesh().bounds();
	EXPECT_EQ(bounds.minimum, (spk::Vector3{0, 0, 0}));
	EXPECT_EQ(bounds.maximum, (spk::Vector3{2, 1, 1}));
}

TEST(MesherGeometry, AirGapPreservesBothIndependentCubeShadowSurfaces)
{
	const auto stone = definition(fixture("cube"), "stone_");
	Volume::Builder builder({3, 1, 1});
	builder.bind(registry({stone}));
	builder.set({0, 0, 0}, 1);
	builder.set({2, 0, 0}, 1);
	const Geometry model = Mesher().bake(builder.build());
	EXPECT_EQ(model.opaqueMesh().indexCount(), 72u);
	EXPECT_NEAR(shadowArea(model), 12.0f, 1.0e-5f);
	expectShadowGeometryMatchesOpaque(model);
}

TEST(MesherGeometry, HalfHeightSlabClipsCubeShadowBoundary)
{
	const auto cube = definition(fixture("cube"), "source_");
	const auto slab = definition(fixture("slab"), "blocker_");
	const Geometry model = Mesher().bake(pair(cube, {}, slab, {}, Face::PositiveX));
	EXPECT_NEAR(generatedBoundaryArea(model, Face::PositiveX), 0.5f, 1.0e-5f);
	EXPECT_NEAR(shadowArea(model), 9.0f, 1.0e-5f);
	expectValidTriangles(model, model.opaqueMesh(), 1.0f);
	expectShadowGeometryMatchesOpaque(model);
}

TEST(MesherGeometry, MixedMaterialOpacityCastsOnlyOpaqueFaces)
{
	Definition::SlotMapping mapping;
	mapping.emplace("side", Definition::Slot{"glass", 0.25f});
	mapping.emplace("top", Definition::Slot{"roof", 1.0f});
	mapping.emplace("bottom", Definition::Slot{"floor", 1.0f});
	const Geometry model = Mesher().bake(single(Definition(fixture("cube"), std::move(mapping))));
	EXPECT_EQ(model.opaqueMesh().indexCount(), 12u);
	EXPECT_EQ(model.transparentMesh().indexCount(), 24u);
	EXPECT_EQ(model.depthMesh().indexCount(), 12u);
	EXPECT_NEAR(shadowArea(model), 2.0f, 1.0e-5f);
	EXPECT_NEAR(visualArea(model.transparentMesh()), 4.0f, 1.0e-5f);
	expectValidTriangles(model, model.opaqueMesh(), 1.0f);
	expectValidTriangles(model, model.transparentMesh(), 0.25f);
	expectShadowGeometryMatchesOpaque(model);
}

TEST(MesherGeometry, TransparentNeighborDoesNotOccludeOpaqueShadow)
{
	const Shape cube = fixture("cube");
	const auto stone = definition(cube, "stone_");
	const auto glass = definition(cube, "glass_", 0.5f);
	const Geometry model = Mesher().bake(pair(stone, {}, glass, {}, Face::PositiveX));
	EXPECT_EQ(model.opaqueMesh().indexCount(), 36u);
	EXPECT_EQ(model.transparentMesh().indexCount(), 30u);
	EXPECT_NEAR(shadowArea(model), 6.0f, 1.0e-5f);
	EXPECT_NEAR(visualArea(model.transparentMesh()), 5.0f, 1.0e-5f);
	expectValidTriangles(model, model.opaqueMesh(), 1.0f);
	expectValidTriangles(model, model.transparentMesh(), 0.5f);
	expectShadowGeometryMatchesOpaque(model);
}

TEST(MesherGeometry, MatchingTranslucentNeighborsCullTheirSharedFaces)
{
	const auto glass = uniformDefinition(fixture("cube"), "glass", 0.5f);
	for (const Face face : Faces)
	{
		SCOPED_TRACE(static_cast<unsigned>(face));
		const auto volume = pair(glass, {}, glass, {}, face);
		const Geometry model = Mesher().bake(volume);
		EXPECT_TRUE(model.opaqueMesh().empty());
		EXPECT_TRUE(model.depthMesh().empty());
		EXPECT_EQ(model.transparentMesh().indexCount(), 60u);
		EXPECT_NEAR(visualArea(model.transparentMesh()), 10.0f, 1.0e-5f);
		expectValidTriangles(model, model.transparentMesh(), 0.5f);
		expectShadowGeometryMatchesOpaque(model);

		Occlusion::Cache cache;
		Mesher cached;
		cached.attachCache(cache);
		expectEqual(model, cached.bake(volume));
	}
}

TEST(MesherGeometry, DifferentTranslucentMaterialsRetainInternalFaces)
{
	const auto cube = fixture("cube");
	const auto blue = uniformDefinition(cube, "blue_glass", 0.5f);
	const auto green = uniformDefinition(cube, "green_glass", 0.5f);
	const Geometry model = Mesher().bake(pair(blue, {}, green, {}, Face::PositiveY));
	EXPECT_TRUE(model.opaqueMesh().empty());
	EXPECT_TRUE(model.depthMesh().empty());
	EXPECT_EQ(model.transparentMesh().indexCount(), 72u);
	EXPECT_NEAR(visualArea(model.transparentMesh()), 12.0f, 1.0e-5f);
	expectValidTriangles(model, model.transparentMesh(), 0.5f);
}

TEST(MesherGeometry, MatchingTranslucentSlabClipsOnlyCoveredInterface)
{
	const auto cube = uniformDefinition(fixture("cube"), "glass", 0.5f);
	const auto slab = uniformDefinition(fixture("slab"), "glass", 0.5f);
	const Geometry model = Mesher().bake(pair(cube, {}, slab, {}, Face::PositiveX));
	EXPECT_TRUE(model.opaqueMesh().empty());
	EXPECT_TRUE(model.depthMesh().empty());
	EXPECT_NEAR(visualArea(model.transparentMesh()), 9.0f, 1.0e-5f);
	expectValidTriangles(model, model.transparentMesh(), 0.5f);
}

TEST(MesherGeometry, CompletelyTransparentGeometryProducesNoShadow)
{
	const Geometry model = Mesher().bake(single(definition(fixture("stair"), "invisible_", 0.0f)));
	EXPECT_TRUE(model.opaqueMesh().empty());
	EXPECT_TRUE(model.depthMesh().empty());
	EXPECT_EQ(model.transparentMesh().indexCount(), 60u);
	expectValidTriangles(model, model.transparentMesh(), 0.0f);
	expectShadowGeometryMatchesOpaque(model);
}

TEST(MesherGeometry, ExternalOpaqueBoundaryRemovesOnlyItsOwnShadowFace)
{
	ExternalMesher mesher;
	mesher.state.state = ExternalMesher::State::Occupied;
	mesher.state.cell = Cell(definition(fixture("cube"), "outside_"));
	const auto source = single(definition(fixture("cube"), "inside_"));
	for (const Face face : Faces)
	{
		SCOPED_TRACE(static_cast<unsigned>(face));
		mesher.onlyFace = face;
		const Geometry model = mesher.bake(source);
		EXPECT_EQ(model.depthMesh().indexCount(), 30u);
		EXPECT_NEAR(shadowArea(model), 5.0f, 1.0e-5f);
		expectShadowGeometryMatchesOpaque(model);
	}
}

TEST(MesherTest, EmptyVolumeBuildsThreeEmptyMeshesAndNoSlots)
{
	Volume::Builder builder({3, 2, 1});
	const auto model = Mesher().bake(builder.build());
	EXPECT_TRUE(model.opaqueMesh().empty());
	EXPECT_TRUE(model.transparentMesh().empty());
	EXPECT_TRUE(model.depthMesh().empty());
	EXPECT_TRUE(model.materialSlots().empty());
	EXPECT_NO_THROW((void)model.opaqueMesh().layout());
}

TEST(MesherTest, CubeHasTwelveTrianglesAndMatchingDepthGeometryWithoutOpenGLContext)
{
	const auto model = Mesher().bake(single(definition(fixture("cube"), "stone_")));
	EXPECT_EQ(model.opaqueMesh().indexCount(), 36u);
	EXPECT_EQ(model.opaqueMesh().vertexCount(), 36u);
	EXPECT_TRUE(model.transparentMesh().empty());
	EXPECT_EQ(model.depthMesh().indexCount(), 36u);
	const auto depth = model.depthMesh().layout().vertexBuffer().cast<spk::Vector3>();
	const auto visual = model.opaqueMesh().layout().vertexBuffer().cast<Geometry::Vertex>();
	ASSERT_EQ(depth.size(), visual.size());
	for (std::size_t i = 0; i < depth.size(); ++i)
	{
		EXPECT_EQ(depth[i], visual[i].position);
	}
	EXPECT_EQ(model.depthMesh().layout().indexBuffer().cast<std::uint32_t>().size(), 36u);
}

TEST(MesherTest, AdjacentOpaqueCubesCullBothSharedFaces)
{
	const Shape cube = fixture("cube");
	const auto volume = pair(definition(cube, "source_"), {}, definition(cube, "blocker_"), {}, Face::PositiveX);
	const auto model = Mesher().bake(volume);
	EXPECT_EQ(model.opaqueMesh().indexCount(), 60u);
	EXPECT_EQ(model.depthMesh().indexCount(), 36u);
	EXPECT_EQ(generatedBoundaryArea(model, Face::PositiveX), 0.0f);
}

TEST(MesherTest, TranslucentPolygonsNeverOccludeOrCastDepth)
{
	const Shape cube = fixture("cube");
	const auto glass = definition(cube, "glass_", 0.5f);
	const auto stone = definition(cube, "stone_");
	const auto translucent = Mesher().bake(single(glass));
	EXPECT_TRUE(translucent.opaqueMesh().empty());
	EXPECT_EQ(translucent.transparentMesh().indexCount(), 36u);
	EXPECT_TRUE(translucent.depthMesh().empty());
	const auto pairModel = Mesher().bake(pair(stone, {}, glass, {}, Face::PositiveX));
	EXPECT_EQ(pairModel.opaqueMesh().indexCount(), 36u);
	EXPECT_EQ(pairModel.transparentMesh().indexCount(), 30u);
	EXPECT_EQ(pairModel.depthMesh().indexCount(), 36u);
}

TEST(MesherTest, OutsideStatesDoNotAssumeUnknownIsOccluding)
{
	ExternalMesher mesher;
	const auto volume = single(definition(fixture("cube"), "source_"));
	EXPECT_EQ(mesher.bake(volume).opaqueMesh().indexCount(), 36u);
	mesher.state.state = ExternalMesher::State::Unknown;
	EXPECT_EQ(mesher.bake(volume).opaqueMesh().indexCount(), 36u);
	mesher.state.state = ExternalMesher::State::Occupied;
	mesher.state.cell = Cell(definition(fixture("cube"), "outside_"));
	EXPECT_EQ(mesher.bake(volume).opaqueMesh().indexCount(), 0u);
	for (Face face : Faces)
	{
		mesher.onlyFace = face;
		EXPECT_EQ(mesher.bake(volume).opaqueMesh().indexCount(), 30u);
	}
	mesher.onlyFace.reset();
	mesher.state.cell = Cell();
	EXPECT_EQ(mesher.bake(volume).opaqueMesh().indexCount(), 36u);
}

TEST(MesherTest, GlobalInnerShellRequiresAllSixOpaqueNeighbors)
{
	for (const auto *name : {"slab", "slope", "stair"})
	{
		for (const auto orientation : Orientations)
		{
			const Shape cube = fixture("cube");
			const Shape shape = fixture(name);
			Volume::Builder builder({3, 3, 3});
			builder.bind(registry({definition(shape, "source_"), definition(cube, "neighbor_")}));
			builder.set({1, 1, 1}, 1, orientation.horizontal, orientation.vertical);
			for (const Face face : Faces)
			{
				builder.set(neighbor(face), 2);
			}
			const Volume sealed = builder.build();
			const Geometry closed = Mesher().bake(sealed);
			for (const auto &vertex : vertices(closed.opaqueMesh()))
			{
				EXPECT_FALSE(closed.materialSlots()[vertex.materialIndex].name.starts_with("source_"));
			}
			Volume::Builder opening(sealed.dimensions());
			opening.load(sealed);
			opening.erase(neighbor(Face::PositiveX));
			const auto open = Mesher().bake(opening.build());
			bool found = false;
			for (const auto &vertex : vertices(open.opaqueMesh()))
			{
				found |= open.materialSlots()[vertex.materialIndex].name.starts_with("source_");
			}
			EXPECT_TRUE(found);
		}
	}
}

TEST(MesherTest, EmptyGeometryKeepsUsedSlotsButNotUnusedDefinitions)
{
	Shape::Builder builder;
	builder.addSlot("hidden");
	const auto hidden = definition(builder.build(), "used_");
	const auto unused = definition(fixture("cube"), "unused_");
	Volume::Builder volume({1, 1, 1});
	volume.bind(registry({hidden, unused}));
	volume.set({0, 0, 0}, 1);
	const Geometry model = Mesher().bake(volume.build());
	EXPECT_TRUE(model.opaqueMesh().empty());
	EXPECT_TRUE(model.transparentMesh().empty());
	EXPECT_TRUE(model.depthMesh().empty());
	ASSERT_EQ(model.materialSlots().size(), 1u);
	EXPECT_EQ(model.materialSlots()[0].name, "used_hidden");
}

TEST(MesherTest, SlotOrderMergeCollisionAndHiddenSlotPreservation)
{
	const Shape cube = fixture("cube");
	const auto stone = definition(cube, "z_");
	const auto other = definition(cube, "a_");
	const Geometry model = Mesher().bake(pair(stone, {}, other, {}, Face::PositiveX));
	ASSERT_EQ(model.materialSlots().size(), 6u);
	for (std::size_t i = 1; i < model.materialSlots().size(); ++i)
	{
		EXPECT_LT(model.materialSlots()[i - 1].name, model.materialSlots()[i].name);
	}
	Definition::SlotMapping same;
	for (const auto &slot : cube.slots())
	{
		same.emplace(slot.name, Definition::Slot{"unified", 1.0f});
	}
	const Definition first(cube, same);
	for (auto &[key, slot] : same)
	{
		(void)key;
		slot.opacity = 0.5f;
	}
	const Definition second(cube, same);
	EXPECT_EQ(Mesher().bake(pair(first, {}, first, {}, Face::PositiveX)).materialSlots().size(), 1u);
	EXPECT_THROW((void)Mesher().bake(pair(first, {}, second, {}, Face::PositiveX)), spk::Exception);
}

TEST(MesherTest, CacheBindingAndDeterminismPreserveFullMeshPayload)
{
	const Shape source = fixture("stair");
	const Shape blocker = fixture("slope");
	const auto volume = pair(definition(source, "source_"), Orientations[5], definition(blocker, "blocker_"), Orientations[3], Face::NegativeY);
	Mesher mesher;
	const auto baseline = mesher.bake(volume);
	expectEqual(baseline, mesher.bake(volume));
	Occlusion::Cache cache;
	mesher.attachCache(cache);
	expectEqual(baseline, mesher.bake(volume));
	expectEqual(baseline, mesher.bake(volume));
	Mesher another;
	another.attachCache(cache);
	expectEqual(baseline, another.bake(volume));
	mesher.detachCache();
	expectEqual(baseline, mesher.bake(volume));
}

TEST(MesherTest, InvalidToleranceDoesNotChangeExistingConfiguration)
{
	Mesher mesher;
	for (float invalid : {0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
	{
		EXPECT_THROW(mesher.setTolerance(invalid), spk::Exception);
		EXPECT_FLOAT_EQ(mesher.tolerance(), 1.0e-5f);
	}
	mesher.setTolerance(2.0e-5f);
	EXPECT_FLOAT_EQ(mesher.tolerance(), 2.0e-5f);
}

TEST(MesherTest, GeometriesSharePublishedImmutableMeshData)
{
	const auto volume = single(definition(fixture("cube"), "source_"));
	Mesher mesher;
	const Geometry first = mesher.bake(volume);
	const Geometry copy = first;
	EXPECT_EQ(&first.opaqueMesh().layout(), &copy.opaqueMesh().layout());
	const auto other = mesher.bake(volume);
	EXPECT_NE(&first.opaqueMesh().layout(), &other.opaqueMesh().layout());
	expectEqual(first, other);
}

TEST(MesherFixtures, EveryShapePairOrientationAndNeighborDirectionMatchesIndependentCoverage)
{
	std::array<Shape, 4> shapes{fixture("cube"), fixture("slab"), fixture("slope"), fixture("stair")};
	std::size_t cases = 0;
	Mesher mesher;
	for (std::size_t s = 0; s < shapes.size(); ++s)
	{
		for (std::size_t b = 0; b < shapes.size(); ++b)
		{
			for (const auto sourceOrientation : Orientations)
			{
				for (const auto blockerOrientation : Orientations)
				{
					for (const auto face : Faces)
					{
						SCOPED_TRACE(::testing::Message() << Names[s] << " vs " << Names[b] << ", face " << static_cast<int>(face));
						const Volume volume = pair(
							definition(shapes[s], "source_"),
							sourceOrientation,
							definition(shapes[b], "blocker_"),
							blockerOrientation,
							face);
						const Geometry model = mesher.bake(volume);
						const float expected = sampledVisibleArea(shapes[s], sourceOrientation, shapes[b], blockerOrientation, face);
						EXPECT_NEAR(generatedBoundaryArea(model, face), expected, 0.05f);
						EXPECT_LE(model.depthMesh().indexCount(), model.opaqueMesh().indexCount());
						expectShadowGeometryMatchesOpaque(model);
						++cases;
					}
				}
			}
		}
	}
	EXPECT_EQ(cases, 4u * 4u * 8u * 8u * 6u);
}

TEST(MesherTest, ClippedQuadPreservesAffineUVNormalsAndTriangleWinding)
{
	Shape::Builder sourceBuilder;
	sourceBuilder.addSlot("face");
	sourceBuilder.addPolygon("face", {{{1, 0, 0}, {0, 0}}, {{1, 1, 0}, {0, 1}}, {{1, 1, 1}, {1, 1}}, {{1, 0, 1}, {1, 0}}});
	Shape::Builder blockerBuilder;
	blockerBuilder.addSlot("face");
	blockerBuilder.addPolygon("face", {{{0, 0, 0}}, {{0, 0, 0.5f}}, {{0, 1, 0.5f}}, {{0, 1, 0}}});
	const auto volume = pair(
		definition(sourceBuilder.build(), "source_"),
		{},
		definition(blockerBuilder.build(), "blocker_"),
		{},
		Face::PositiveX);
	const Geometry model = Mesher().bake(volume);
	EXPECT_NEAR(generatedBoundaryArea(model, Face::PositiveX), 0.5f, 1.0e-5f);
	const auto view = model.opaqueMesh().layout().vertexBuffer().cast<Geometry::Vertex>();
	const auto triangles = model.opaqueMesh().layout().indexBuffer().cast<std::uint32_t>();
	std::size_t visibleSource = 0;
	for (std::size_t i = 0; i < triangles.size(); i += 3)
	{
		const auto &a = view[triangles[i]];
		const auto &b = view[triangles[i + 1]];
		const auto &c = view[triangles[i + 2]];
		const auto cross = (b.position - a.position).cross(c.position - a.position);
		EXPECT_GT(cross.length(), 2.0e-10);
		if (!model.materialSlots()[a.materialIndex].name.starts_with("source_"))
		{
			continue;
		}
		++visibleSource;
		EXPECT_GT(cross.x, 0.0f);
		for (const auto &vertex : {a, b, c})
		{
			EXPECT_EQ(vertex.normal, spk::Vector3(1, 0, 0));
			EXPECT_NEAR(vertex.uv.x, vertex.position.z - 1.0f, 1.0e-5f);
			EXPECT_NEAR(vertex.uv.y, vertex.position.y - 1.0f, 1.0e-5f);
		}
	}
	EXPECT_GT(visibleSource, 0u);
}

TEST(MesherMaterialIndices, BakeUsesLocalIndicesForOpaqueAndTranslucentGeometry)
{
	Definition::SlotMapping mapping;
	mapping.emplace("side", Definition::Slot{"glass", 0.25f});
	mapping.emplace("top", Definition::Slot{"roof", 1.0f});
	mapping.emplace("bottom", Definition::Slot{"floor", 1.0f});
	const auto volume = single(Definition(fixture("cube"), std::move(mapping)));
	const Geometry geometry = Mesher().bake(volume);
	ASSERT_EQ(geometry.materialSlots().size(), 3u);
	EXPECT_EQ(geometry.materialSlots()[0].name, "floor");
	EXPECT_EQ(geometry.materialSlots()[1].name, "glass");
	EXPECT_EQ(geometry.materialSlots()[2].name, "roof");
	for (const auto &vertex : vertices(geometry.opaqueMesh()))
	{
		ASSERT_LT(vertex.materialIndex, geometry.materialSlots().size());
		EXPECT_FLOAT_EQ(geometry.materialSlot(vertex.materialIndex).opacity, 1.0f);
	}
	for (const auto &vertex : vertices(geometry.transparentMesh()))
	{
		ASSERT_LT(vertex.materialIndex, geometry.materialSlots().size());
		EXPECT_EQ(geometry.materialSlot(vertex.materialIndex).name, "glass");
	}
	expectShadowGeometryMatchesOpaque(Geometry(geometry));
	EXPECT_THROW((void)geometry.materialSlot(3), spk::Exception);
}

TEST(MesherMaterialIndices, UnusedSlotsArePreservedWithoutMaterialIndexOffset)
{
	const Geometry geometry({}, {}, {}, {{"hidden", 1.0f}, {"unseen", 0.4f}});
	EXPECT_TRUE(geometry.opaqueMesh().empty());
	EXPECT_TRUE(geometry.transparentMesh().empty());
	EXPECT_TRUE(geometry.depthMesh().empty());
	EXPECT_EQ(geometry.materialSlot(0).name, "hidden");
	EXPECT_EQ(geometry.materialSlot(1).name, "unseen");
	EXPECT_THROW((void)geometry.materialSlot(2), spk::Exception);
}

TEST(MesherMaterialIndices, GeometryCopiesKeepTheirOriginalSlotIndices)
{
	const Geometry source = Mesher().bake(single(definition(fixture("cube"), "stone_")));
	const Geometry copy = source;
	EXPECT_EQ(&copy.opaqueMesh().layout(), &source.opaqueMesh().layout());
	EXPECT_EQ(&copy.depthMesh().layout(), &source.depthMesh().layout());
	for (const auto &vertex : vertices(copy.opaqueMesh()))
	{
		EXPECT_LT(vertex.materialIndex, copy.materialSlots().size());
		EXPECT_EQ(copy.materialSlot(vertex.materialIndex).name, source.materialSlot(vertex.materialIndex).name);
	}
}

TEST(MesherTest, ConcurrentBakesCanShareAnOcclusionCache)
{
	const auto cube = definition(fixture("cube"), "stone_");
	const auto slab = definition(fixture("slab"), "stone_");
	const auto volume = pair(cube, {}, slab, {}, Face::PositiveX);
	const auto expected = Mesher().bake(volume);
	Occlusion::Cache cache;
	std::vector<std::future<Geometry>> jobs;
	for (int index = 0; index < 8; ++index)
	{
		jobs.push_back(std::async(std::launch::async, [&]() {
			Mesher mesher;
			mesher.attachCache(cache);
			return mesher.bake(volume);
		}));
	}
	for (auto &job : jobs)
	{
		expectEqual(expected, job.get());
	}
}

TEST(MesherTest, SparseRegistryIdsAliasingOneDefinitionPreserveOrientedGeometry)
{
	const auto shared = definition(fixture("stair"), "shared_", 0.5f);
	Definition::Registry::Builder sparse;
	sparse.add(7, shared);
	sparse.add(Volume::PackedCell::MaxDefinitionId, shared);
	sparse.add(12345, definition(fixture("cube"), "unused_"));
	const auto sparseRegistry = sparse.build();
	const auto denseRegistry = registry({shared});
	for (const auto orientationValue : Orientations)
	{
		Volume::Builder actual({3, 2, 1});
		actual.bind(sparseRegistry);
		actual.set({0, 0, 0}, 7, orientationValue.horizontal, orientationValue.vertical);
		actual.set({1, 0, 0}, Volume::PackedCell::MaxDefinitionId, orientationValue.horizontal, orientationValue.vertical);
		Volume::Builder expected({3, 2, 1});
		expected.bind(denseRegistry);
		expected.set({0, 0, 0}, 1, orientationValue.horizontal, orientationValue.vertical);
		expected.set({1, 0, 0}, 1, orientationValue.horizontal, orientationValue.vertical);
		expectEqual(Mesher().bake(actual.build()), Mesher().bake(expected.build()));
	}
}
