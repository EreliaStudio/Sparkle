#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/engine_headless.hpp"
#include "engine/entity3d.hpp"
#include "engine/registry.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "geometry/shadow_mesh.hpp"
#include "geometry/texture_mesh_3d.hpp"
#include "math/vector4.hpp"
#include "rendering/engine/engine_view.hpp"
#include "rendering/engine/rendering_component.hpp"
#include "shadow_map/contributor.hpp"
#include "shadow_map/mesh_filter.hpp"

namespace
{
	using spk::ShadowMap::Contributor;
	using spk::ShadowMap::Edition;
	using spk::ShadowMap::Mesh;
	using spk::ShadowMap::MeshFilter;
	using spk::ShadowMap::MeshInstance;
	using spk::ShadowMap::ShadowMode;
	using spk::Matrix4x4;

	constexpr float NaN = std::numeric_limits<float>::quiet_NaN();
	constexpr float Infinity = std::numeric_limits<float>::infinity();

	[[nodiscard]] spk::TextureMesh3D makeSourceMesh()
	{
		spk::TextureMesh3D::Builder builder;
		const auto a = builder.addVertex({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}});
		const auto b = builder.addVertex({{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}});
		const auto c = builder.addVertex({{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}});
		const auto d = builder.addVertex({{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
		builder.addTriangle(a, c, b);
		builder.addTriangle(a, d, c);
		return std::move(builder).build();
	}

	[[nodiscard]] Mesh makeShadowMesh()
	{
		const auto source = makeSourceMesh();
		return Mesh::from(source, [](const spk::Texture3DVertex &vertex) {
			return vertex.position;
		});
	}

	[[nodiscard]] Matrix4x4 invalidMatrix(float value, std::size_t column, std::size_t row)
	{
		auto matrix = Matrix4x4::identity();
		matrix[column][row] = value;
		return matrix;
	}
}

TEST(ShadowMapMeshTest, FromGenericVerticesPreservesWindingIndicesAndSharedVertices)
{
	const auto source = makeSourceMesh();
	const auto shadow = Mesh::from(source, [](const spk::Texture3DVertex &vertex) {
		return vertex.position;
	});

	EXPECT_EQ(shadow.vertexCount(), 4u);
	EXPECT_EQ(shadow.indexCount(), 6u);
	EXPECT_EQ(shadow.layout().vertexBuffer().stride(), sizeof(spk::Vector3));
	const auto positions = shadow.layout().vertexBuffer().cast<spk::Vector3>();
	EXPECT_EQ(positions[2], (spk::Vector3{1.0f, 1.0f, 0.0f}));
	const auto indices = shadow.layout().indexBuffer().cast<std::uint32_t>();
	const auto original = source.layout().indexBuffer().cast<std::uint32_t>();
	for (std::size_t i = 0; i < indices.size(); ++i)
	{
		EXPECT_EQ(indices[i], original[i]);
	}
	EXPECT_EQ(indices[0], 0u);
	EXPECT_EQ(indices[1], 2u);
	EXPECT_EQ(indices[2], 1u);
	EXPECT_EQ(indices[3], 0u);
	EXPECT_EQ(indices[4], 3u);
	EXPECT_EQ(indices[5], 2u);
}

TEST(ShadowMapMeshTest, RejectsIncompleteTrianglesInvalidIndicesAndNonFiniteCoordinates)
{
	const auto extractor = [](const spk::Texture3DVertex &vertex) {
		return vertex.position;
	};

	spk::TextureMesh3D::Builder incompleteBuilder;
	(void)incompleteBuilder.addVertex({{1.0f, 2.0f, 3.0f}, {}, {}});
	incompleteBuilder.addIndex(0);
	auto incomplete = std::move(incompleteBuilder).build();
	EXPECT_THROW((void)Mesh::from(incomplete, extractor), spk::Exception);

	auto invalidIndex = makeSourceMesh();
	auto &indexBuffer = const_cast<spk::IndexBuffer &>(invalidIndex.layout().indexBuffer());
	indexBuffer.cast<std::uint32_t>()[2] = 100u; // Simulate malformed external mesh data.
	EXPECT_THROW((void)Mesh::from(invalidIndex, extractor), spk::Exception);

	for (float value : {NaN, Infinity, -Infinity})
	{
		spk::TextureMesh3D::Builder invalidVertexBuilder;
		(void)invalidVertexBuilder.addVertex({{value, 0.0f, 0.0f}, {}, {}});
		auto invalidVertex = std::move(invalidVertexBuilder).build();
		// Even an unindexed mesh must not silently accept non-finite positions.
		EXPECT_THROW((void)Mesh::from(invalidVertex, extractor), spk::Exception);
	}
}

TEST(ShadowMapMeshTest, EmptyMeshIsAllowedAndCopiesKeepImmutableContentAlive)
{
	const spk::TextureMesh3D neverBuilt;
	const auto empty = Mesh::from(neverBuilt, [](const spk::Texture3DVertex &v) {
		return v.position;
	});
	EXPECT_TRUE(empty.empty());
	EXPECT_EQ(empty.indexCount(), 0u);

	Mesh retained;
	{
		const auto source = makeSourceMesh();
		retained = Mesh::from(source, [](const spk::Texture3DVertex &v) {
			return v.position;
		});
	}
	Mesh copy = retained;
	EXPECT_EQ(std::addressof(copy.layout()), std::addressof(retained.layout()));
	EXPECT_EQ(copy.layout().vertexBuffer().cast<spk::Vector3>()[1], (spk::Vector3{1.0f, 0.0f, 0.0f}));
}

TEST(ShadowMapMeshFilterTest, DefaultAndEmptyBindRestoreIdentity)
{
	MeshFilter filter;
	EXPECT_TRUE(filter.instances().empty());
	EXPECT_EQ(filter.baseTransform(), Matrix4x4::identity());

	const auto mesh = makeShadowMesh();
	const auto base = Matrix4x4::translation(4.0f, 5.0f, 6.0f);
	filter.bind(mesh, base);
	ASSERT_EQ(filter.instances().size(), 1u);
	EXPECT_EQ(filter.instances()[0].localTransform, Matrix4x4::identity());
	EXPECT_EQ(filter.baseTransform(), base);

	filter.bind(MeshFilter::Collection{});
	EXPECT_TRUE(filter.instances().empty());
	EXPECT_EQ(filter.baseTransform(), Matrix4x4::identity());

	filter.bind(mesh, base);
	filter.bind(MeshFilter::Collection{}, base);
	EXPECT_TRUE(filter.instances().empty());
	EXPECT_EQ(filter.baseTransform(), Matrix4x4::identity());

	filter.bind(mesh, base);
	filter.unbind();
	EXPECT_TRUE(filter.instances().empty());
	EXPECT_EQ(filter.baseTransform(), Matrix4x4::identity());
}

TEST(ShadowMapMeshFilterTest, SharedMeshInstancesComposeEntityBaseAndLocalInThatOrder)
{
	auto shared = makeShadowMesh();
	const auto first = Matrix4x4::translation(1.0f, 0.0f, 0.0f);
	const auto second = Matrix4x4::translation(-3.0f, 0.0f, 0.0f);
	const auto base = Matrix4x4::scale(2.0f, 2.0f, 2.0f);
	MeshFilter filter;
	filter.bind(MeshFilter::Collection{{shared, first}, {shared, second}}, base);

	ASSERT_EQ(filter.instances().size(), 2u);
	EXPECT_EQ(std::addressof(filter.instances()[0].mesh.layout()), std::addressof(filter.instances()[1].mesh.layout()));
	EXPECT_EQ(filter.instances()[0].localTransform, first);
	EXPECT_EQ(filter.instances()[1].localTransform, second);

	spk::Entity3D entity("caster");
	entity.transform().place({10.0f, 0.0f, 0.0f});
	const auto p = spk::Vector4{0.0f, 0.0f, 0.0f, 1.0f};
	const auto firstWorld = (entity.transform().modelMatrix() * filter.baseTransform() * filter.instances()[0].localTransform) * p;
	const auto secondWorld = (entity.transform().modelMatrix() * filter.baseTransform() * filter.instances()[1].localTransform) * p;
	EXPECT_FLOAT_EQ(firstWorld.x, 12.0f);
	EXPECT_FLOAT_EQ(secondWorld.x, 4.0f);
	EXPECT_FLOAT_EQ(firstWorld.w, 1.0f);
}

TEST(ShadowMapMeshFilterTest, EachSuccessfulBindNotifiesExactlyOnceEvenWhenIdentical)
{
	MeshFilter filter;
	int notifications = 0;
	auto contract = filter.subscribeToEdition([&](const MeshFilter &modified) {
		EXPECT_EQ(&modified, &filter);
		++notifications;
	});
	const auto mesh = makeShadowMesh();
	const auto matrix = Matrix4x4::translation(2.0f, 0.0f, 0.0f);
	const MeshFilter::Collection collection{{mesh, Matrix4x4::identity()}, {mesh, matrix}};

	filter.bind(mesh);
	EXPECT_EQ(notifications, 1);
	filter.bind(mesh);
	EXPECT_EQ(notifications, 2);
	filter.bind(mesh, matrix);
	EXPECT_EQ(notifications, 3);
	filter.bind(collection, matrix);
	EXPECT_EQ(notifications, 4);
	filter.bind(collection, matrix);
	EXPECT_EQ(notifications, 5);
	filter.bind(collection);
	EXPECT_EQ(notifications, 6);
	EXPECT_EQ(filter.baseTransform(), Matrix4x4::identity());
	filter.unbind();
	EXPECT_EQ(notifications, 7);
	filter.unbind();
	EXPECT_EQ(notifications, 8);
	EXPECT_TRUE(contract.isValid());
}

TEST(ShadowMapMeshFilterTest, InvalidBaseAndInstanceMatricesLeaveBothFieldsAndNotificationsUntouched)
{
	MeshFilter filter;
	const auto mesh = makeShadowMesh();
	const auto base = Matrix4x4::translation(3.0f, 4.0f, 5.0f);
	const MeshFilter::Collection initial{{mesh, Matrix4x4::translation(1.0f, 2.0f, 3.0f)}};
	filter.bind(initial, base);
	int notifications = 0;
	auto contract = filter.subscribeToEdition([&](const MeshFilter &) {
		++notifications;
	});

	for (float invalid : {NaN, Infinity, -Infinity})
	{
		EXPECT_THROW(filter.bind(mesh, invalidMatrix(invalid, 3, 2)), spk::Exception);
		EXPECT_THROW(filter.bind(MeshFilter::Collection{{mesh, invalidMatrix(invalid, 1, 3)}}, base), spk::Exception);
		EXPECT_THROW(filter.bind(MeshFilter::Collection{}, invalidMatrix(invalid, 0, 0)), spk::Exception);
		EXPECT_EQ(filter.baseTransform(), base);
		ASSERT_EQ(filter.instances().size(), 1u);
		EXPECT_EQ(filter.instances()[0].localTransform, initial[0].localTransform);
		EXPECT_EQ(std::addressof(filter.instances()[0].mesh.layout()), std::addressof(mesh.layout()));
		EXPECT_EQ(notifications, 0);
	}
}

TEST(ShadowMapMeshFilterTest, OldCollectionsRemainReadableAfterReplacementAndSourcesDestruction)
{
	MeshFilter filter;
	MeshFilter::Collection snapshot;
	{
		const auto source = makeSourceMesh();
		const auto depth = Mesh::from(source, [](const spk::Texture3DVertex &v) { return v.position; });
		filter.bind(MeshFilter::Collection{{depth, Matrix4x4::translation(1.0f, 0.0f, 0.0f)}});
		snapshot = filter.instances();
	}
	ASSERT_EQ(snapshot.size(), 1u);
	EXPECT_EQ(snapshot[0].mesh.indexCount(), 6u);
	filter.bind(Mesh{});
	ASSERT_EQ(filter.instances().size(), 1u);
	EXPECT_TRUE(filter.instances()[0].mesh.empty());
	EXPECT_EQ(snapshot[0].mesh.layout().indexBuffer().cast<std::uint32_t>()[1], 2u);
}

TEST(ShadowMapContributorTest, InitialStateAndRegistryFollowEngineContext)
{
	using Registry = spk::Registry<Contributor, spk::EngineHeadless *>;
	static_assert(!std::is_base_of_v<spk::EngineView::Contributor, Contributor>);
	spk::EngineHeadless engine;
	spk::Entity3D entity("caster");
	auto &contributor = entity.addComponent<Contributor>();
	EXPECT_EQ(contributor.owner(), &entity);
	EXPECT_EQ(contributor.shadowMode(), ShadowMode::None);
	EXPECT_TRUE(contributor.meshFilter().instances().empty());
	EXPECT_EQ(contributor.meshFilter().baseTransform(), Matrix4x4::identity());
	EXPECT_TRUE(contributor.layers().empty());
	EXPECT_TRUE(Registry::instance().elements(nullptr).contains(&contributor));

	engine.addEntity(&entity);
	EXPECT_TRUE(Registry::instance().elements(&engine).contains(&contributor));
	EXPECT_FALSE(Registry::instance().elements(nullptr).contains(&contributor));
	engine.removeEntity(&entity);
	EXPECT_FALSE(Registry::instance().elements(&engine).contains(&contributor));
	EXPECT_TRUE(Registry::instance().elements(nullptr).contains(&contributor));
}

TEST(ShadowMapContributorTest, ModeAndBindingNotificationsExposeCorrectInvalidationScope)
{
	Contributor contributor;
	std::vector<Edition> events;
	auto contract = contributor.subscribeToEdition([&](const Contributor &source, const Edition &edition) {
		EXPECT_EQ(&source, &contributor);
		events.push_back(edition);
	});

	contributor.setShadowMode(ShadowMode::None);
	EXPECT_TRUE(events.empty());
	contributor.bind(makeShadowMesh());
	ASSERT_EQ(events.size(), 1u);
	EXPECT_EQ(events.back().kind, Edition::Kind::Binding);
	EXPECT_FALSE(events.back().invalidatesStatic);
	EXPECT_FALSE(events.back().invalidatesFinal);

	contributor.setShadowMode(ShadowMode::Dynamic);
	ASSERT_EQ(events.size(), 2u);
	EXPECT_EQ(events.back().kind, Edition::Kind::ShadowMode);
	EXPECT_EQ(events.back().previousMode, ShadowMode::None);
	EXPECT_EQ(events.back().currentMode, ShadowMode::Dynamic);
	EXPECT_TRUE(events.back().invalidatesStatic);
	EXPECT_TRUE(events.back().invalidatesFinal);
	contributor.setShadowMode(ShadowMode::Dynamic);
	EXPECT_EQ(events.size(), 2u);

	contributor.meshFilter().bind(makeShadowMesh());
	ASSERT_EQ(events.size(), 3u);
	EXPECT_EQ(events.back().kind, Edition::Kind::Binding);
	EXPECT_FALSE(events.back().invalidatesStatic);
	EXPECT_TRUE(events.back().invalidatesFinal);

	contributor.setShadowMode(ShadowMode::Static);
	ASSERT_EQ(events.size(), 4u);
	EXPECT_EQ(events.back().previousMode, ShadowMode::Dynamic);
	EXPECT_EQ(events.back().currentMode, ShadowMode::Static);
	EXPECT_TRUE(events.back().invalidatesStatic);
	EXPECT_TRUE(events.back().invalidatesFinal);
	contributor.bind(makeShadowMesh());
	ASSERT_EQ(events.size(), 5u);
	EXPECT_TRUE(events.back().invalidatesStatic);
	EXPECT_TRUE(events.back().invalidatesFinal);

	contributor.setShadowMode(ShadowMode::None);
	ASSERT_EQ(events.size(), 6u);
	EXPECT_TRUE(events.back().invalidatesStatic);
	EXPECT_TRUE(events.back().invalidatesFinal);
	EXPECT_TRUE(contract.isValid());
}

TEST(ShadowMapContributorTest, ParentAndChildShadowModesNeverInherit)
{
	spk::Entity3D parent("parent");
	spk::Entity3D child("child", &parent);
	auto &p = parent.addComponent<Contributor>();
	auto &c = child.addComponent<Contributor>();
	c.setShadowMode(ShadowMode::Dynamic);
	EXPECT_EQ(p.shadowMode(), ShadowMode::None);
	EXPECT_EQ(c.shadowMode(), ShadowMode::Dynamic);
	p.setShadowMode(ShadowMode::Static);
	EXPECT_EQ(c.shadowMode(), ShadowMode::Dynamic);
	c.setShadowMode(ShadowMode::None);
	EXPECT_EQ(p.shadowMode(), ShadowMode::Static);
	p.setShadowMode(ShadowMode::None);
	EXPECT_EQ(c.shadowMode(), ShadowMode::None);
}

TEST(ShadowMapContributorTest, LayersAreSeparateFromVisualComponentsAndNotifyOnChange)
{
	spk::Entity3D entity("caster");
	auto &visual = entity.addComponent<spk::RenderingComponent3D>();
	auto &shadow = entity.addComponent<Contributor>();
	visual.setLayers({"visual"});
	int count = 0;
	std::vector<Edition> events;
	auto contract = shadow.subscribeToEdition([&](const Contributor &, const Edition &edition) {
		++count;
		events.push_back(edition);
	});
	shadow.setLayers({"shadow"});
	shadow.setLayers({"shadow"});
	EXPECT_EQ(count, 1);
	EXPECT_EQ(events[0].kind, Edition::Kind::Layers);
	EXPECT_EQ(shadow.layers(), (Contributor::Layers{"shadow"}));
	EXPECT_EQ(visual.layers(), (spk::EngineView::Layers{"visual"}));

	spk::EngineView::CaptureContext view;
	EXPECT_TRUE(view.isLayerCompatible(shadow.layers()));
	view.layers = spk::EngineView::Layers{"visual"};
	EXPECT_FALSE(view.isLayerCompatible(shadow.layers()));
	view.layers = spk::EngineView::Layers{"shadow"};
	EXPECT_TRUE(view.isLayerCompatible(shadow.layers()));
	shadow.setLayers({});
	EXPECT_EQ(count, 2);
	EXPECT_FALSE(view.isLayerCompatible(shadow.layers()));
	EXPECT_TRUE(contract.isValid());
}

TEST(ShadowMapContributorTest, TransformNotificationsRemainObservableForFutureShadowSystem)
{
	spk::Entity3D parent("parent");
	spk::Entity3D child("child", &parent);
	auto &shadow = child.addComponent<Contributor>();
	shadow.setShadowMode(ShadowMode::Dynamic);
	int edits = 0;
	auto contract = parent.transform().subscribeToEdition([&](const spk::Transform3D &) {
		++edits;
	});
	parent.transform().place({2.0f, 0.0f, 0.0f});
	EXPECT_EQ(edits, 1);
	const auto world = child.transform().modelMatrix() * spk::Vector4{0.0f, 0.0f, 0.0f, 1.0f};
	EXPECT_FLOAT_EQ(world.x, 2.0f);
	EXPECT_EQ(shadow.shadowMode(), ShadowMode::Dynamic);
	EXPECT_TRUE(contract.isValid());
}
