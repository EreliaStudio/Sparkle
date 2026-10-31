#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "exception.hpp"
#include "voxel/voxel_mesher.hpp"

namespace
{
	using namespace spk::Voxel;
	using Face = Shape::Face;
	using Position = spk::Vector3UInt;
	using Horizontal = Shape::HorizontalOrientation;
	using Vertical = Shape::VerticalOrientation;

	constexpr std::array<const char *, 4> Names{"cube", "slab", "slope", "stair"};
	constexpr std::array<Face, 6> Faces{Face::NegativeX, Face::PositiveX, Face::NegativeY, Face::PositiveY, Face::NegativeZ, Face::PositiveZ};
	constexpr std::array<Shape::Orientation, 8> Orientations{{
		{Horizontal::XPositive, Vertical::YPositive},
		{Horizontal::ZPositive, Vertical::YPositive},
		{Horizontal::XNegative, Vertical::YPositive},
		{Horizontal::ZNegative, Vertical::YPositive},
		{Horizontal::XPositive, Vertical::YNegative},
		{Horizontal::ZPositive, Vertical::YNegative},
		{Horizontal::XNegative, Vertical::YNegative},
		{Horizontal::ZNegative, Vertical::YNegative}}};

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

	[[nodiscard]] std::vector<MeshVertex> vertices(const VoxelMesh &mesh)
	{
		const auto view = mesh.layout().vertexBuffer().cast<MeshVertex>();
		return {view.begin(), view.end()};
	}

	[[nodiscard]] std::vector<std::uint32_t> indices(const VoxelMesh &mesh)
	{
		const auto view = mesh.layout().indexBuffer().cast<std::uint32_t>();
		return {view.begin(), view.end()};
	}

	void expectEqual(const Model &a, const Model &b)
	{
		ASSERT_EQ(a.materialSlots().size(), b.materialSlots().size());
		for (std::size_t i = 0; i < a.materialSlots().size(); ++i)
		{
			EXPECT_EQ(a.materialSlots()[i].name, b.materialSlots()[i].name);
			EXPECT_FLOAT_EQ(a.materialSlots()[i].opacity, b.materialSlots()[i].opacity);
		}
		auto compare = [](const VoxelMesh &left, const VoxelMesh &right) {
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
				for (const auto &polygon : blocker.instance(blockerOrientation).polygons())
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
				for (const auto &polygon : source.instance(sourceOrientation).polygons())
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

	[[nodiscard]] float generatedBoundaryArea(const Model &model, Face face)
	{
		const auto verticesView = model.opaqueMesh().layout().vertexBuffer().cast<MeshVertex>();
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

	class ExternalMesher final : public VoxelMesher
	{
	public:
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

TEST(VoxelMesherTest, EmptyVolumeBuildsThreeEmptyMeshesAndNoSlots)
{
	Volume::Builder builder({3, 2, 1});
	const auto model = VoxelMesher().bake(builder.build());
	EXPECT_TRUE(model.opaqueMesh().empty());
	EXPECT_TRUE(model.transparentMesh().empty());
	EXPECT_TRUE(model.depthMesh().empty());
	EXPECT_TRUE(model.materialSlots().empty());
	EXPECT_NO_THROW((void)model.opaqueMesh().layout());
}

TEST(VoxelMesherTest, CubeHasTwelveTrianglesAndMatchingDepthGeometryWithoutOpenGLContext)
{
	const auto model = VoxelMesher().bake(single(definition(fixture("cube"), "stone_")));
	EXPECT_EQ(model.opaqueMesh().indexCount(), 36u);
	EXPECT_EQ(model.opaqueMesh().vertexCount(), 36u);
	EXPECT_TRUE(model.transparentMesh().empty());
	EXPECT_EQ(model.depthMesh().indexCount(), 36u);
	const auto depth = model.depthMesh().layout().vertexBuffer().cast<spk::Vector3>();
	const auto visual = model.opaqueMesh().layout().vertexBuffer().cast<MeshVertex>();
	ASSERT_EQ(depth.size(), visual.size());
	for (std::size_t i = 0; i < depth.size(); ++i)
	{
		EXPECT_EQ(depth[i], visual[i].position);
	}
	EXPECT_EQ(model.depthMesh().layout().indexBuffer().cast<std::uint32_t>().size(), 36u);
}

TEST(VoxelMesherTest, AdjacentOpaqueCubesCullBothSharedFaces)
{
	const Shape cube = fixture("cube");
	const auto volume = pair(definition(cube, "source_"), {}, definition(cube, "blocker_"), {}, Face::PositiveX);
	const auto model = VoxelMesher().bake(volume);
	EXPECT_EQ(model.opaqueMesh().indexCount(), 60u);
	EXPECT_EQ(model.depthMesh().indexCount(), 60u);
	EXPECT_EQ(generatedBoundaryArea(model, Face::PositiveX), 0.0f);
}

TEST(VoxelMesherTest, TranslucentPolygonsNeverOccludeOrCastDepth)
{
	const Shape cube = fixture("cube");
	const auto glass = definition(cube, "glass_", 0.5f);
	const auto stone = definition(cube, "stone_");
	const auto translucent = VoxelMesher().bake(single(glass));
	EXPECT_TRUE(translucent.opaqueMesh().empty());
	EXPECT_EQ(translucent.transparentMesh().indexCount(), 36u);
	EXPECT_TRUE(translucent.depthMesh().empty());
	const auto pairModel = VoxelMesher().bake(pair(stone, {}, glass, {}, Face::PositiveX));
	EXPECT_EQ(pairModel.opaqueMesh().indexCount(), 36u);
	EXPECT_EQ(pairModel.transparentMesh().indexCount(), 36u);
	EXPECT_EQ(pairModel.depthMesh().indexCount(), 36u);
}

TEST(VoxelMesherTest, OutsideStatesDoNotAssumeUnknownIsOccluding)
{
	ExternalMesher mesher;
	const auto volume = single(definition(fixture("cube"), "source_"));
	EXPECT_EQ(mesher.bake(volume).opaqueMesh().indexCount(), 36u);
	mesher.state.state = OutsideCell::State::Unknown;
	EXPECT_EQ(mesher.bake(volume).opaqueMesh().indexCount(), 36u);
	mesher.state.state = OutsideCell::State::Occupied;
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

TEST(VoxelMesherTest, GlobalInnerShellRequiresAllSixOpaqueNeighbors)
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
			const Model closed = VoxelMesher().bake(sealed);
			for (const auto &vertex : vertices(closed.opaqueMesh()))
			{
				EXPECT_FALSE(closed.materialSlots()[vertex.materialIndex].name.starts_with("source_"));
			}
			Volume::Builder opening(sealed.dimensions());
			opening.load(sealed);
			opening.erase(neighbor(Face::PositiveX));
			const auto open = VoxelMesher().bake(opening.build());
			bool found = false;
			for (const auto &vertex : vertices(open.opaqueMesh()))
			{
				found |= open.materialSlots()[vertex.materialIndex].name.starts_with("source_");
			}
			EXPECT_TRUE(found);
		}
	}
}

TEST(VoxelMesherTest, SlotOrderMergeCollisionAndHiddenSlotPreservation)
{
	const Shape cube = fixture("cube");
	const auto stone = definition(cube, "z_");
	const auto other = definition(cube, "a_");
	const Model model = VoxelMesher().bake(pair(stone, {}, other, {}, Face::PositiveX));
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
	EXPECT_EQ(VoxelMesher().bake(pair(first, {}, first, {}, Face::PositiveX)).materialSlots().size(), 1u);
	EXPECT_THROW((void)VoxelMesher().bake(pair(first, {}, second, {}, Face::PositiveX)), spk::Exception);
}

TEST(VoxelMesherTest, CacheBindingAndDeterminismPreserveFullMeshPayload)
{
	const Shape source = fixture("stair");
	const Shape blocker = fixture("slope");
	const auto volume = pair(definition(source, "source_"), Orientations[5], definition(blocker, "blocker_"), Orientations[3], Face::NegativeY);
	VoxelMesher mesher;
	const auto baseline = mesher.bake(volume);
	expectEqual(baseline, mesher.bake(volume));
	Occlusion::Cache cache;
	mesher.bind(&cache);
	expectEqual(baseline, mesher.bake(volume));
	expectEqual(baseline, mesher.bake(volume));
	VoxelMesher another;
	another.bind(&cache);
	expectEqual(baseline, another.bake(volume));
	mesher.bind(nullptr);
	expectEqual(baseline, mesher.bake(volume));
}

TEST(VoxelMesherTest, InvalidToleranceDoesNotChangeExistingConfiguration)
{
	VoxelMesher mesher;
	for (float invalid : {0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
	{
		EXPECT_THROW(mesher.setTolerance(invalid), spk::Exception);
		EXPECT_FLOAT_EQ(mesher.tolerance(), 1.0e-5f);
	}
	mesher.setTolerance(2.0e-5f);
	EXPECT_FLOAT_EQ(mesher.tolerance(), 2.0e-5f);
}

TEST(VoxelMesherTest, ModelsSharePublishedImmutableMeshData)
{
	const auto volume = single(definition(fixture("cube"), "source_"));
	VoxelMesher mesher;
	const Model first = mesher.bake(volume);
	const Model copy = first;
	EXPECT_EQ(&first.opaqueMesh().layout(), &copy.opaqueMesh().layout());
	const auto other = mesher.bake(volume);
	EXPECT_NE(&first.opaqueMesh().layout(), &other.opaqueMesh().layout());
	expectEqual(first, other);
}

TEST(VoxelMesherFixtures, EveryShapePairOrientationAndNeighborDirectionMatchesIndependentCoverage)
{
	std::array<Shape, 4> shapes{fixture("cube"), fixture("slab"), fixture("slope"), fixture("stair")};
	std::size_t cases = 0;
	VoxelMesher mesher;
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
						const Model model = mesher.bake(volume);
						const float expected = sampledVisibleArea(shapes[s], sourceOrientation, shapes[b], blockerOrientation, face);
						EXPECT_NEAR(generatedBoundaryArea(model, face), expected, 0.05f);
						EXPECT_EQ(model.depthMesh().indexCount(), model.opaqueMesh().indexCount());
						++cases;
					}
				}
			}
		}
	}
	EXPECT_EQ(cases, 4u * 4u * 8u * 8u * 6u);
}

TEST(VoxelMesherTest, ClippedQuadPreservesAffineUVNormalsAndTriangleWinding)
{
	Shape::Builder sourceBuilder;
	sourceBuilder.addSlot("face");
	sourceBuilder.addPolygon("face", {
		{{1, 0, 0}, {0, 0}},
		{{1, 1, 0}, {0, 1}},
		{{1, 1, 1}, {1, 1}},
		{{1, 0, 1}, {1, 0}}});
	Shape::Builder blockerBuilder;
	blockerBuilder.addSlot("face");
	blockerBuilder.addPolygon("face", {
		{{0, 0, 0}},
		{{0, 0, 0.5f}},
		{{0, 1, 0.5f}},
		{{0, 1, 0}}});
	const auto volume = pair(
		definition(sourceBuilder.build(), "source_"),
		{},
		definition(blockerBuilder.build(), "blocker_"),
		{},
		Face::PositiveX);
	const Model model = VoxelMesher().bake(volume);
	EXPECT_NEAR(generatedBoundaryArea(model, Face::PositiveX), 0.5f, 1.0e-5f);
	const auto view = model.opaqueMesh().layout().vertexBuffer().cast<MeshVertex>();
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
