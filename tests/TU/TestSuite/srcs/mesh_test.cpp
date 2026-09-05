#include <gtest/gtest.h>

#include <cstdint>
#include <utility>

#include "geometry/color_mesh_2d.hpp"
#include "geometry/mesh.hpp"
#include "geometry/texture_mesh_2d.hpp"
#include "geometry/texture_mesh_3d.hpp"

namespace
{
	struct SimpleVertex
	{
		float x;
		float y;
	};

	class SimpleMesh final : public spk::Mesh<SimpleVertex>
	{
	public:
		class Builder final : public spk::Mesh<SimpleVertex>::Builder<SimpleMesh>
		{
		private:
			void _setupAttributes(spk::VertexBuffer &buffer) override
			{
				buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 2);
			}
		};
	};

	template <typename MeshType>
	void expectStandardAttribute(
		const MeshType &mesh,
		std::size_t index,
		std::uint32_t location,
		std::uint32_t components,
		std::size_t offset)
	{
		const auto attributes = mesh.layout().vertexBuffer().attributes();
		ASSERT_GT(attributes.size(), index);
		EXPECT_EQ(attributes[index].attribute.location, location);
		EXPECT_EQ(attributes[index].attribute.type, spk::VertexBuffer::Attribute::Type::Float);
		EXPECT_EQ(attributes[index].attribute.componentCount, components);
		EXPECT_EQ(attributes[index].offset, offset);
	}
}

TEST(MeshTest, DefaultMeshIsEmptyAndHasNoReadableLayout)
{
	SimpleMesh mesh;
	EXPECT_TRUE(mesh.empty());
	EXPECT_EQ(mesh.vertexCount(), 0u);
	EXPECT_EQ(mesh.indexCount(), 0u);
	EXPECT_EQ(mesh.indexType(), spk::IndexBuffer::Type::UnsignedInt);
	EXPECT_THROW((void)mesh.layout(), std::logic_error);
}

TEST(MeshTest, BuilderReservesBuildsQuadAndPreservesManualIndexOrder)
{
	SimpleMesh::Builder builder;
	builder.reserve(8, 12);
	const auto first = builder.addVertex({0.0f, 0.0f});
	const auto second = builder.addVertex({1.0f, 0.0f});
	const auto third = builder.addVertex({0.0f, 1.0f});
	builder.addTriangle(third, second, first);
	auto mesh = std::move(builder).build();

	EXPECT_EQ(mesh.vertexCount(), 3u);
	EXPECT_EQ(mesh.indexCount(), 3u);
	EXPECT_FALSE(mesh.empty());
	EXPECT_EQ(mesh.layout().vertexBuffer().stride(), sizeof(SimpleVertex));
	EXPECT_EQ(mesh.layout().vertexBuffer().cast<SimpleVertex>()[1].x, 1.0f);
	const auto indices = mesh.layout().indexBuffer().cast<std::uint32_t>();
	EXPECT_EQ(indices[0], third);
	EXPECT_EQ(indices[1], second);
	EXPECT_EQ(indices[2], first);
}

TEST(MeshTest, EmptyBuiltMeshHasLayoutAndSharedCopiesRetainContent)
{
	SimpleMesh mesh = std::move(SimpleMesh::Builder{}).build();
	EXPECT_TRUE(mesh.empty());
	EXPECT_NO_THROW((void)mesh.layout());
	EXPECT_EQ(mesh.layout().vertexBuffer().stride(), sizeof(SimpleVertex));

	SimpleMesh copy = mesh;
	EXPECT_EQ(copy.vertexCount(), 0u);
	EXPECT_EQ(&copy.layout(), &mesh.layout());
}

TEST(MeshTest, MissingVertexIndicesAndConsumedBuilderAreRejected)
{
	SimpleMesh::Builder builder;
	EXPECT_THROW(builder.addIndex(0), std::out_of_range);
	(void)builder.addVertex({0.0f, 0.0f});
	EXPECT_THROW(builder.addIndex(1), std::out_of_range);
	auto mesh = std::move(builder).build();
	EXPECT_THROW((void)builder.addVertex({1.0f, 1.0f}), std::logic_error);
	EXPECT_THROW(builder.reserve(1, 1), std::logic_error);
}

TEST(ColorMesh2DTest, QuadRetainsPayloadIndicesAndExpectedAttributeLayout)
{
	spk::ColorMesh2D::Builder builder;
	const spk::Color red{1.0f, 0.0f, 0.0f, 0.5f};
	builder.addShape(
		{{0.0f, 0.0f}, -1.0f, red},
		{{0.0f, 2.0f}, -1.0f, red},
		{{3.0f, 2.0f}, -1.0f, red},
		{{3.0f, 0.0f}, -1.0f, red});
	auto mesh = std::move(builder).build();

	EXPECT_EQ(mesh.vertexCount(), 4u);
	EXPECT_EQ(mesh.indexCount(), 6u);
	const auto vertices = mesh.layout().vertexBuffer().cast<spk::ColorVertex2D>();
	EXPECT_EQ(vertices[2].position, spk::Vector2(3.0f, 2.0f));
	EXPECT_FLOAT_EQ(vertices[2].depth, -1.0f);
	EXPECT_EQ(vertices[2].color, red);
	expectStandardAttribute(mesh, 0, 0, 2, 0);
	expectStandardAttribute(mesh, 1, 1, 1, sizeof(float) * 2);
	expectStandardAttribute(mesh, 2, 2, 4, sizeof(float) * 3);
	EXPECT_EQ(mesh.layout().vertexBuffer().stride(), sizeof(spk::ColorVertex2D));
}

TEST(ColorMesh2DTest, OneVertexAndMultipleShapesRetainCountsAcrossCopiesAndMoves)
{
	spk::ColorMesh2D::Builder builder;
	(void)builder.addVertex({{1.0f, 2.0f}, 3.0f, {0.1f, 0.2f, 0.3f, 0.4f}});
	auto oneVertex = std::move(builder).build();
	EXPECT_EQ(oneVertex.vertexCount(), 1u);
	EXPECT_TRUE(oneVertex.empty());
	spk::ColorMesh2D copy = oneVertex;
	spk::ColorMesh2D moved = std::move(copy);
	EXPECT_EQ(moved.vertexCount(), 1u);
}

TEST(TextureMesh2DTest, PayloadAllowsBoundaryAndOutsideUVsWithExpectedLayout)
{
	spk::TextureMesh2D::Builder builder;
	const auto a = builder.addVertex({{0.0f, 0.0f}, 0.25f, {-1.0f, 0.0f}});
	const auto b = builder.addVertex({{2.0f, 0.0f}, 0.5f, {1.0f, 0.0f}});
	const auto c = builder.addVertex({{0.0f, 2.0f}, 0.75f, {0.0f, 2.0f}});
	builder.addTriangle(a, b, c);
	auto mesh = std::move(builder).build();
	const auto vertices = mesh.layout().vertexBuffer().cast<spk::Texture2DVertex>();
	EXPECT_EQ(vertices[0].uv, spk::Vector2(-1.0f, 0.0f));
	EXPECT_EQ(vertices[2].uv, spk::Vector2(0.0f, 2.0f));
	expectStandardAttribute(mesh, 0, 0, 2, 0);
	expectStandardAttribute(mesh, 1, 1, 1, sizeof(float) * 2);
	expectStandardAttribute(mesh, 2, 2, 2, sizeof(float) * 3);
	EXPECT_EQ(mesh.layout().vertexBuffer().stride(), sizeof(spk::Texture2DVertex));
}

TEST(TextureMesh3DTest, PayloadNormalsWindingUVsAndLayoutArePreserved)
{
	spk::TextureMesh3D::Builder builder;
	const auto a = builder.addVertex({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});
	const auto b = builder.addVertex({{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 2.0f}, {1.0f, 0.0f}});
	const auto c = builder.addVertex({{0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}});
	builder.addTriangle(a, c, b);
	auto mesh = std::move(builder).build();

	const auto vertices = mesh.layout().vertexBuffer().cast<spk::Texture3DVertex>();
	EXPECT_EQ(vertices[0].normal, spk::Vector3(0.0f, 0.0f, 0.0f));
	EXPECT_EQ(vertices[1].normal, spk::Vector3(0.0f, 0.0f, 2.0f));
	const auto indices = mesh.layout().indexBuffer().cast<std::uint32_t>();
	EXPECT_EQ(indices[0], a);
	EXPECT_EQ(indices[1], c);
	EXPECT_EQ(indices[2], b);
	expectStandardAttribute(mesh, 0, 0, 3, 0);
	expectStandardAttribute(mesh, 1, 1, 3, sizeof(float) * 3);
	expectStandardAttribute(mesh, 2, 2, 2, sizeof(float) * 6);
	EXPECT_EQ(mesh.layout().vertexBuffer().stride(), sizeof(spk::Texture3DVertex));
}

TEST(MeshTest, DISABLED_VertexIndexOverflowNeedsDeterministicCapacitySeam)
{
	GTEST_SKIP() << "Reaching more than uint32_t vertices would require impractical memory; Builder exposes no injectable count seam.";
}

TEST(MeshTest, DISABLED_RenderingRequiresProgramAndFramebufferIntegrationFixture)
{
	GTEST_SKIP() << "Mesh owns buffer layout only; rendering requires Program and render-command fixtures covered by later integration targets.";
}
