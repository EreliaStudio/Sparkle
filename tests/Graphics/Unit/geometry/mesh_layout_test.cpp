#include <gtest/gtest.h>

#include "geometry/texture_mesh_3d.hpp"
#include "graphics/opengl/mesh_layout.hpp"

namespace
{
	spk::TextureMesh3D makeMesh()
	{
		spk::TextureMesh3D::Builder builder;
		const auto a = builder.addVertex({{0, 0, 0}, {0, 0, 1}, {0, 0}});
		const auto b = builder.addVertex({{1, 0, 0}, {0, 0, 1}, {1, 0}});
		const auto c = builder.addVertex({{0, 1, 0}, {0, 0, 1}, {0, 1}});
		builder.addTriangle(a, b, c);
		return std::move(builder).build();
	}
}

TEST(MeshLayoutTest, ConfiguresAndBindsPublishedBuffersWithoutCopying)
{
	auto mesh = makeMesh();
	const auto &layout = spk::MeshLayout::obtain(mesh);
	EXPECT_EQ(layout.vertexBuffer().buffer(), &mesh.vertexBuffer());
	EXPECT_EQ(layout.indexBuffer().buffer(), &mesh.indexBuffer());
	EXPECT_EQ(layout.vertexBuffer().stride(), sizeof(spk::Texture3DVertex));
	EXPECT_EQ(layout.vertexBuffer().attributes().size(), 3u);
	EXPECT_EQ(layout.indexBuffer().type(), spk::IndexBuffer::Type::UnsignedInt);
	EXPECT_EQ(layout.indexBuffer().count(), 3u);
}

TEST(MeshLayoutTest, SharedMeshCopiesReuseLayoutAndRetainBuffers)
{
	auto mesh = makeMesh();
	auto copy = mesh;
	const auto &layout = spk::MeshLayout::obtain(mesh);
	const auto identifier = layout.vertexBuffer().identifier();
	mesh = {};
	EXPECT_EQ(&spk::MeshLayout::obtain(copy), &layout);
	EXPECT_EQ(layout.vertexBuffer().identifier(), identifier);
	EXPECT_EQ(layout.vertexBuffer().buffer(), &copy.vertexBuffer());
}

TEST(MeshLayoutTest, DistinctMeshesUseDistinctResources)
{
	const auto first = makeMesh();
	const auto second = makeMesh();
	EXPECT_NE(spk::MeshLayout::obtain(first).vertexBuffer().identifier(), spk::MeshLayout::obtain(second).vertexBuffer().identifier());
	EXPECT_THROW((void)spk::MeshLayout::obtain(spk::TextureMesh3D{}), std::logic_error);
}
