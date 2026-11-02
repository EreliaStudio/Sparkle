#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "voxel/mesher.hpp"
#include "voxel/model.hpp"
#include "voxel/palette.hpp"

namespace
{
	using Model = spk::Voxel::Model;
	using SurfaceMesh = spk::Voxel::Mesh::SurfaceMesh;
	using Mesh = spk::Voxel::Mesh;
	using Vertex = spk::Voxel::Mesh::Vertex;
	using Matrix = spk::Matrix4x4;

	[[nodiscard]] Model model(const std::string &slot, std::uint32_t index = 0)
	{
		SurfaceMesh::Builder builder;
		const Vertex vertex{{0, 0, 0}, {0, 1, 0}, {0, 0}, index};
		const auto a = builder.addVertex(vertex);
		const auto b = builder.addVertex(vertex);
		const auto c = builder.addVertex(vertex);
		builder.addTriangle(a, b, c);
		return Model(Mesh(std::move(builder).build(), {}, {}, {{slot, 1.0f}}, index));
	}

	[[nodiscard]] spk::Palette palette(const spk::Texture &atlas, std::vector<std::string> names)
	{
		spk::Voxel::Palette::Builder builder;
		for (const auto &name : names)
		{
			spk::Material::Builder material;
			material.add("albedo", atlas);
			builder.setMaterial(name, material.build());
		}
		return builder.build();
	}

	void expectMatrix(const Matrix &actual, const Matrix &expected)
	{
		for (std::size_t c = 0; c < 4; ++c)
		{
			for (std::size_t r = 0; r < 4; ++r)
			{
				EXPECT_NEAR(actual[c][r], expected[c][r], 0.0001f);
			}
		}
	}

	[[nodiscard]] std::uint32_t materialIndex(const Model &value)
	{
		return value.opaqueMesh().layout().vertexBuffer().cast<Vertex>()[0].materialIndex;
	}
}

TEST(VoxelModelTest, MeshCarriesGeometryAndMaterialSlots)
{
	const auto source = model("cloth", 5);
	EXPECT_EQ(source.mesh().opaqueMesh().indexCount(), 3u);
	EXPECT_TRUE(source.mesh().transparentMesh().empty());
	EXPECT_EQ(source.materialIndexOffset(), 5u);
	EXPECT_EQ(source.materialSlot(5).name, "cloth");
	EXPECT_THROW((void)source.materialSlot(4), spk::Exception);
}

TEST(VoxelModelTest, BuilderOwnsMutationsAndPublishedModelsStayImmutable)
{
	Model::Builder builder;
	EXPECT_THROW(builder.addAnchor(""), spk::Exception);
	builder.addAnchor("shoulder", Matrix::translation(2, 0, 0));
	EXPECT_THROW(builder.addAnchor("shoulder"), spk::Exception);
	const Model published = builder.build();
	EXPECT_TRUE(published.hasAnchor("shoulder"));
	expectMatrix(published.anchor("shoulder").transform(), Matrix::translation(2, 0, 0));
	builder.setAnchorTransform("shoulder", Matrix::translation(3, 0, 0));
	EXPECT_THROW(builder.setAnchorTransform("missing", Matrix::identity()), spk::Exception);
	const Model changed = builder.build();
	expectMatrix(changed.anchor("shoulder").transform(), Matrix::translation(3, 0, 0));
	expectMatrix(published.anchor("shoulder").transform(), Matrix::translation(2, 0, 0));
	builder.removeAnchor("shoulder");
	EXPECT_THROW(builder.removeAnchor("shoulder"), spk::Exception);
	EXPECT_FALSE(builder.build().hasAnchor("shoulder"));
	EXPECT_TRUE(published.hasAnchor("shoulder"));
}

TEST(VoxelModelTest, AnchorsRequireRigidFiniteMatrices)
{
	Model::Builder builder;
	const auto invalid = Matrix::scale(2, 1, 1);
	EXPECT_THROW(builder.addAnchor("bad", invalid), spk::Exception);
	builder.addAnchor("good");
	EXPECT_THROW(builder.setAnchorTransform("good", invalid), spk::Exception);
	expectMatrix(builder.build().anchor("good").transform(), Matrix::identity());
}

TEST(VoxelModelTest, BindingPreservesParentOnInvalidAttachment)
{
	Model::Builder arm;
	arm.setMesh(model("skin").mesh());
	arm.addAnchor("shoulder", Matrix::translation(1, 0, 0));
	Model::Builder torso;
	torso.addAnchor("leftArm");
	torso.bind("leftArm", arm.build(), "shoulder");
	const Model published = torso.build();
	EXPECT_EQ(published.anchor("leftArm").attachment(), "shoulder");
	ASSERT_NE(published.anchor("leftArm").model(), nullptr);
	EXPECT_THROW(torso.bind("leftArm", arm.build(), "missing"), spk::Exception);
	EXPECT_EQ(torso.build().anchor("leftArm").attachment(), "shoulder");
	torso.unbind("leftArm");
	EXPECT_EQ(torso.build().anchor("leftArm").model(), nullptr);
	EXPECT_EQ(torso.build().anchor("leftArm").attachment(), Model::Anchor::NoAttachmentPoint);
	EXPECT_NE(published.anchor("leftArm").model(), nullptr);
}

TEST(VoxelModelTest, CopiesShareImmutableHierarchies)
{
	Model::Builder builder;
	builder.addAnchor("arm");
	builder.bind("arm", model("skin"));
	const Model original = builder.build();
	const Model copy = original;
	EXPECT_EQ(&original.anchors(), &copy.anchors());
	ASSERT_NE(copy.anchor("arm").model(), nullptr);
	EXPECT_EQ(&copy.anchor("arm").model()->opaqueMesh(), &original.anchor("arm").model()->opaqueMesh());
	builder.addAnchor("new");
	EXPECT_FALSE(copy.hasAnchor("new"));
	EXPECT_TRUE(builder.build().hasAnchor("new"));
}

TEST(VoxelModelViewTest, IndependentPalettesForEachModel)
{
	spk::Texture atlas;
	Model::Builder torsoBuilder(model("cloth"));
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.bind("leftArm", model("skin"));
	Model::View view(torsoBuilder.build());
	EXPECT_FALSE(view.isRenderable());
	view.setPalette(palette(atlas, {"cloth"}), Model::PalettePropagation::Self);
	view.setPalette("leftArm", palette(atlas, {"skin"}));
	EXPECT_TRUE(view.isRenderable());
	ASSERT_NE(view.palette(), nullptr);
	ASSERT_NE(view.palette("leftArm"), nullptr);
	EXPECT_NE(&view.palette()->materials(), &view.palette("leftArm")->materials());
}

TEST(VoxelModelViewTest, RecursivePaletteBindingIsAtomic)
{
	spk::Texture atlas;
	Model::Builder torsoBuilder(model("cloth"));
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.bind("leftArm", model("skin"));
	Model::View view(torsoBuilder.build());
	const auto complete = palette(atlas, {"cloth", "skin"});
	view.setPalette(complete);
	EXPECT_TRUE(view.isRenderable());
	EXPECT_THROW(view.setPalette(palette(atlas, {"cloth"})), spk::Exception);
	EXPECT_EQ(&view.palette()->materials(), &complete.materials());
	EXPECT_EQ(&view.palette("leftArm")->materials(), &complete.materials());
}

TEST(VoxelModelViewTest, TwoViewsShareMeshesAndHaveIndependentAppearances)
{
	spk::Texture first;
	spk::Texture second;
	const auto source = model("stone");
	Model::View red(source, palette(first, {"stone"}));
	Model::View blue(source, palette(second, {"stone"}));
	EXPECT_EQ(&red.model().opaqueMesh().layout(), &blue.model().opaqueMesh().layout());
	const auto redAtlas = red.palette()->find("stone")->find("albedo")->texture.identifier();
	const auto blueAtlas = blue.palette()->find("stone")->find("albedo")->texture.identifier();
	EXPECT_NE(redAtlas, blueAtlas);
}

TEST(VoxelModelViewTest, ChildJointRotationKeepsAttachmentAligned)
{
	Model::Builder torsoBuilder;
	torsoBuilder.addAnchor("shoulder", Matrix::translation(2, 0, 0));
	Model::Builder armBuilder;
	armBuilder.addAnchor("origin", Matrix::translation(1, 0, 0));
	torsoBuilder.bind("shoulder", armBuilder.build(), "origin");
	Model::View view(torsoBuilder.build());
	view.setAnchorRotation("shoulder", spk::Quaternion::fromEuler({0, 0, 90}));
	const auto rotation = Matrix::rotation(view.anchorRotation("shoulder"));
	const auto expected = Matrix::translation(2, 0, 0) * rotation * Matrix::translation(-1, 0, 0);
	std::vector<Matrix> transforms;
	view.visit([&](const Model &, const spk::Palette &, const Matrix &matrix) {
		transforms.push_back(matrix);
	});
	ASSERT_EQ(transforms.size(), 2u);
	expectMatrix(transforms[1], expected);
	const auto joint = transforms[1] * Matrix::translation(1, 0, 0);
	expectMatrix(joint, Matrix::translation(2, 0, 0) * rotation);
}

TEST(VoxelModelViewTest, AnimationsStayIndependentBetweenViews)
{
	Model::Builder torsoBuilder;
	torsoBuilder.addAnchor("arm");
	torsoBuilder.bind("arm", Model{});
	Model::View first(torsoBuilder.build());
	Model::View second(torsoBuilder.build());
	first.setAnchorRotation("arm", spk::Quaternion::fromEuler({0, 0, 45}));
	EXPECT_NE(first.anchorRotation("arm"), second.anchorRotation("arm"));
	EXPECT_EQ(second.anchorRotation("arm"), spk::Quaternion::identity());
}

TEST(VoxelModelViewTest, DuplicateAnchorNamesRejectAmbiguousLookup)
{
	Model::Builder torsoBuilder;
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.addAnchor("rightArm");
	Model::Builder armBuilder;
	armBuilder.addAnchor("elbow");
	torsoBuilder.bind("leftArm", armBuilder.build());
	torsoBuilder.bind("rightArm", armBuilder.build());
	Model::View view(torsoBuilder.build());
	EXPECT_THROW(view.setAnchorRotation("elbow", spk::Quaternion::identity()), spk::Exception);
}

TEST(VoxelModelBuilderTest, HarmonizeCombinesSlotsAndPreservesSources)
{
	Model::Builder builder(model("cloth", 5));
	builder.addAnchor("arm");
	builder.bind("arm", model("skin"));
	const Model before = builder.build();
	builder.harmonize();
	const Model result = builder.build();
	EXPECT_EQ(result.materialIndexOffset(), 0u);
	EXPECT_EQ(result.materialSlots().size(), 2u);
	EXPECT_EQ(result.materialSlots()[0].name, "cloth");
	EXPECT_EQ(result.materialSlots()[1].name, "skin");
	EXPECT_EQ(materialIndex(result), 0u);
	ASSERT_NE(result.anchor("arm").model(), nullptr);
	EXPECT_EQ(materialIndex(*result.anchor("arm").model()), 1u);
	EXPECT_EQ(materialIndex(before), 5u);
	EXPECT_EQ(materialIndex(*before.anchor("arm").model()), 0u);
	spk::Texture atlas;
	Model::View view(result, palette(atlas, {"cloth", "skin"}));
	EXPECT_TRUE(view.isRenderable());
}

TEST(VoxelModelBuilderTest, HarmonizeUnifiesSameNameAndOpacity)
{
	Model::Builder builder(model("cloth", 5));
	builder.addAnchor("left");
	builder.addAnchor("right");
	builder.bind("left", model("cloth", 8));
	builder.bind("right", model("cloth", 2));
	builder.harmonize();
	const Model result = builder.build();
	EXPECT_EQ(result.materialSlots().size(), 1u);
	EXPECT_EQ(materialIndex(result), 0u);
	EXPECT_EQ(materialIndex(*result.anchor("left").model()), 0u);
	EXPECT_EQ(materialIndex(*result.anchor("right").model()), 0u);
}

TEST(VoxelModelBuilderTest, HarmonizeRejectsOpacityConflictsAtomically)
{
	Model::Builder builder(model("cloth"));
	builder.addAnchor("arm");
	builder.bind("arm", Model(Mesh({}, {}, {}, {{"cloth", 0.5f}})));
	const Model before = builder.build();
	EXPECT_THROW(builder.harmonize(), spk::Exception);
	ASSERT_EQ(builder.build().materialSlots().size(), before.materialSlots().size());
	EXPECT_EQ(builder.build().materialSlots()[0].name, before.materialSlots()[0].name);
	EXPECT_EQ(materialIndex(builder.build()), 0u);
}

TEST(VoxelModelBuilderTest, HarmonizeIsIdempotent)
{
	Model::Builder builder(model("cloth", 7));
	builder.addAnchor("arm");
	builder.bind("arm", model("skin", 3));
	builder.harmonize();
	const auto first = builder.build();
	builder.harmonize();
	const auto second = builder.build();
	EXPECT_EQ(first.materialSlots().size(), second.materialSlots().size());
	EXPECT_EQ(materialIndex(first), materialIndex(second));
	EXPECT_EQ(materialIndex(*first.anchor("arm").model()), materialIndex(*second.anchor("arm").model()));
}

TEST(VoxelMaterialLayoutTest, RequiresSortedUniqueNames)
{
	EXPECT_THROW((void)spk::Voxel::MaterialLayout({"skin", "cloth"}), spk::Exception);
	EXPECT_THROW((void)spk::Voxel::MaterialLayout({"cloth", "cloth"}), spk::Exception);
	spk::Voxel::MaterialLayout layout({"cloth", "skin"});
	EXPECT_EQ(layout.names().size(), 2u);
}

TEST(VoxelAnchorPathTest, VectorOfSegmentsKeepsLiteralNames)
{
	using Path = Model::Anchor::Path;
	const Path nested{"leftShoulder", "elbow"};
	const Path literal{"leftShoulder/elbow"};
	EXPECT_NE(nested, literal);
	EXPECT_EQ(nested.size(), 2u);
	ASSERT_EQ(literal.size(), 1u);
	EXPECT_EQ(literal.front(), "leftShoulder/elbow");
}

TEST(VoxelModelViewTest, TypedPathsResolveRepeatedAnchorNames)
{
	using Path = Model::Anchor::Path;
	Model::Builder torsoBuilder;
	Model::Builder armBuilder;
	armBuilder.addAnchor("elbow");
	torsoBuilder.addAnchor("leftShoulder");
	torsoBuilder.addAnchor("rightShoulder");
	torsoBuilder.bind("leftShoulder", armBuilder.build());
	torsoBuilder.bind("rightShoulder", armBuilder.build());
	Model::View view(torsoBuilder.build());
	const auto left = Path{"leftShoulder", "elbow"};
	const auto right = Path{"rightShoulder", "elbow"};
	const auto rotation = spk::Quaternion::fromEuler({0, 0, 45});
	view.setAnchorRotation(left, rotation);
	EXPECT_EQ(view.anchorRotation(left), rotation.normalized());
	EXPECT_EQ(view.anchorRotation(right), spk::Quaternion::identity());
	EXPECT_THROW(view.setAnchorRotation("elbow", rotation), spk::Exception);
	EXPECT_THROW(view.setAnchorRotation(Path{"leftShoulder", "missing"}, rotation), spk::Exception);
}

TEST(VoxelModelViewTest, AnchorHandleAppliesRotationOnlyToOwningView)
{
	using Path = Model::Anchor::Path;
	Model::Builder torsoBuilder;
	torsoBuilder.addAnchor("arm");
	torsoBuilder.bind("arm", Model{});
	Model::View first(torsoBuilder.build());
	Model::View second(torsoBuilder.build());
	auto handle = first.anchor(Path{"arm"});
	const auto rotation = spk::Quaternion::fromEuler({0, 0, 60});
	EXPECT_EQ(handle.path(), (Path{"arm"}));
	handle.setRotation(rotation);
	EXPECT_EQ(handle.rotation(), rotation.normalized());
	EXPECT_EQ(second.anchorRotation("arm"), spk::Quaternion::identity());
	handle.resetRotation();
	EXPECT_EQ(first.anchorRotation("arm"), spk::Quaternion::identity());
}

TEST(VoxelModelViewTest, ShortNameLookupRequiresUniqueness)
{
	Model::Builder torsoBuilder;
	Model::Builder armBuilder;
	armBuilder.addAnchor("elbow");
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.bind("leftArm", armBuilder.build());
	Model::View single(torsoBuilder.build());
	EXPECT_NO_THROW((void)single.anchor("elbow"));
	torsoBuilder.addAnchor("rightArm");
	torsoBuilder.bind("rightArm", armBuilder.build());
	Model::View duplicate(torsoBuilder.build());
	EXPECT_THROW((void)duplicate.anchor("elbow"), spk::Exception);
	EXPECT_NO_THROW((void)duplicate.anchor(Model::Anchor::Path{"leftArm", "elbow"}));
}

TEST(VoxelModelViewTest, ExplicitPathPreservesSlashInAnAnchorName)
{
	using Path = Model::Anchor::Path;
	Model::Builder torsoBuilder;
	Model::Builder armBuilder;
	armBuilder.addAnchor("elbow");
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.addAnchor("leftArm/elbow");
	torsoBuilder.bind("leftArm", armBuilder.build());
	Model::View view(torsoBuilder.build());
	const auto rotation = spk::Quaternion::fromEuler({0, 0, 33});
	view.anchor(Path{"leftArm/elbow"}).setRotation(rotation);
	EXPECT_EQ(view.anchorRotation(Path{"leftArm/elbow"}), rotation.normalized());
	EXPECT_EQ(view.anchorRotation(Path{"leftArm", "elbow"}), spk::Quaternion::identity());
}

TEST(VoxelModelViewTest, TypedPathsApplyIndependentChildPalettes)
{
	using Path = Model::Anchor::Path;
	spk::Texture atlas;
	Model::Builder armBuilder(model("skin"));
	armBuilder.addAnchor("elbow");
	armBuilder.bind("elbow", model("metal"));
	Model::Builder torsoBuilder;
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.addAnchor("rightArm");
	torsoBuilder.bind("leftArm", armBuilder.build());
	torsoBuilder.bind("rightArm", armBuilder.build());
	Model::View view(torsoBuilder.build());
	const auto skin = palette(atlas, {"skin"});
	const auto metal = palette(atlas, {"metal"});
	view.setPalette(Path{"leftArm"}, skin);
	view.setPalette(Path{"rightArm"}, skin);
	view.setPalette(Path{"leftArm", "elbow"}, metal);
	view.setPalette(Path{"rightArm", "elbow"}, metal);
	ASSERT_NE(view.palette(Path{"leftArm", "elbow"}), nullptr);
	EXPECT_EQ(&view.palette(Path{"leftArm", "elbow"})->materials(), &metal.materials());
	EXPECT_TRUE(view.isRenderable());
}

TEST(VoxelModelViewTest, HandleFailsSafelyAfterViewDestructionAndMove)
{
	using Path = Model::Anchor::Path;
	Model::Builder sourceBuilder;
	sourceBuilder.addAnchor("arm");
	auto view = std::make_unique<Model::View>(sourceBuilder.build());
	auto destroyed = view->anchor(Path{"arm"});
	view.reset();
	EXPECT_THROW((void)destroyed.rotation(), spk::Exception);
	Model::View original(sourceBuilder.build());
	auto movedHandle = original.anchor(Path{"arm"});
	Model::View moved(std::move(original));
	EXPECT_THROW((void)movedHandle.rotation(), spk::Exception);
	EXPECT_NO_THROW((void)moved.anchor(Path{"arm"}));
}

TEST(VoxelModelViewTest, CopiedViewHasIndependentHandlesAndAssignmentInvalidatesThem)
{
	using Path = Model::Anchor::Path;
	Model::Builder sourceBuilder;
	sourceBuilder.addAnchor("arm");
	Model::View original(sourceBuilder.build());
	auto originalHandle = original.anchor(Path{"arm"});
	Model::View copy(original);
	const auto rotation = spk::Quaternion::fromEuler({0, 0, 35});
	copy.anchor(Path{"arm"}).setRotation(rotation);
	EXPECT_EQ(originalHandle.rotation(), spk::Quaternion::identity());
	auto oldCopyHandle = copy.anchor(Path{"arm"});
	copy = original;
	EXPECT_THROW((void)oldCopyHandle.rotation(), spk::Exception);
	EXPECT_EQ(originalHandle.rotation(), spk::Quaternion::identity());
}
