#include "structure/engine/spk_collision_mesh_tester.hpp"
#include "structure/engine/spk_obj_mesh.hpp"

TEST_F(CollisionMeshTest, MergeTrianglesIntoQuad) // NOLINT
{
	spk::ObjMesh obj;
	spk::Vertex v1;
	v1.position = {0.0f, 0.0f, 0.0f};
	spk::Vertex v2;
	v2.position = {1.0f, 0.0f, 0.0f};
	spk::Vertex v3;
	v3.position = {1.0f, 1.0f, 0.0f};
	spk::Vertex v4;
	v4.position = {0.0f, 1.0f, 0.0f};
	obj.addShape(v1, v2, v3);
	obj.addShape(v1, v3, v4);

	spk::SafePointer<spk::ObjMesh> ptr(&obj);
	spk::CollisionMesh mesh = spk::CollisionMesh::fromObjMesh(ptr);

	ASSERT_EQ(mesh.polygons().size(), 1);
	const auto &wire = mesh.polygons()[0].pointsRef();
	EXPECT_EQ(wire.size(), 4u);

	auto copy = mesh.polygons()[0];
	auto before = copy.pointsRef();
	auto after = copy.rewind();
	EXPECT_EQ(after, before);
}

TEST_F(CollisionMeshTest, SplitConcave) // NOLINT
{
	spk::ObjMesh obj;
	spk::Vertex a;
	a.position = {0.0f, 0.0f, 0.0f};
	spk::Vertex b;
	b.position = {2.0f, 0.0f, 0.0f};
	spk::Vertex c;
	c.position = {1.0f, 1.0f, 0.0f};
	spk::Vertex d;
	d.position = {0.0f, 2.0f, 0.0f};
	spk::Vertex e;
	e.position = {2.0f, 2.0f, 0.0f};
	obj.addShape(a, b, c);
	obj.addShape(a, c, d);
	obj.addShape(d, c, e);

	spk::SafePointer<spk::ObjMesh> ptr(&obj);
	spk::CollisionMesh mesh = spk::CollisionMesh::fromObjMesh(ptr);

	EXPECT_GT(mesh.polygons().size(), 1u);
	for (const auto &poly : mesh.polygons())
	{
		EXPECT_TRUE(poly.isConvex());
		auto copy = poly;
		auto before = copy.pointsRef();
		auto after = copy.rewind();
		EXPECT_EQ(after, before);
	}
}
