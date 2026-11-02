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
	using SurfaceMesh = spk::Voxel::Geometry::SurfaceMesh;
	using Geometry = spk::Voxel::Geometry;
	using Vertex = spk::Voxel::Geometry::Vertex;
	using Matrix = spk::Matrix4x4;

	[[nodiscard]] Model model(const std::string &slot)
	{
		SurfaceMesh::Builder builder;
		const Vertex vertex{{0, 0, 0}, {0, 1, 0}, {0, 0}, 0};
		const auto a = builder.addVertex(vertex);
		const auto b = builder.addVertex(vertex);
		const auto c = builder.addVertex(vertex);
		builder.addTriangle(a, b, c);
		return Model(Geometry(std::move(builder).build(), {}, {}, {{slot, 1.0f}}));
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

TEST(VoxelModelTest, AnchorDistinguishesDetachedFromAttachedEmptyModel)
{
	Model::Builder builder;
	builder.addAnchor("hand");
	EXPECT_EQ(builder.build().anchor("hand").model(), nullptr);
	builder.attach("hand", Model{});
	const Model attached = builder.build();
	ASSERT_NE(attached.anchor("hand").model(), nullptr);
	EXPECT_TRUE(attached.anchor("hand").model()->materialSlots().empty());
	builder.detach("hand");
	EXPECT_EQ(builder.build().anchor("hand").model(), nullptr);
	EXPECT_NE(attached.anchor("hand").model(), nullptr);
}

TEST(VoxelModelTest, AttachedModelRetainsDataAfterSourceDestruction)
{
	Model::Builder builder;
	builder.addAnchor("arm");
	{
		Model source = model("skin");
		builder.attach("arm", source);
	}
	const Model torso = builder.build();
	ASSERT_NE(torso.anchor("arm").model(), nullptr);
	EXPECT_EQ(torso.anchor("arm").model()->materialSlot(0).name, "skin");
	Model::Builder harmonized(torso);
	harmonized.harmonize();
	const Model result = harmonized.build();
	ASSERT_NE(result.anchor("arm").model(), nullptr);
	EXPECT_EQ(result.anchor("arm").model()->materialSlot(0).name, "skin");
	EXPECT_NE(torso.anchor("arm").model(), nullptr);
}

TEST(VoxelModelTest, MeshCarriesGeometryAndMaterialSlots)
{
	const auto source = model("cloth");
	EXPECT_EQ(source.geometry().opaqueMesh().indexCount(), 3u);
	EXPECT_TRUE(source.geometry().transparentMesh().empty());
	EXPECT_EQ(materialIndex(source), 0u);
	EXPECT_EQ(source.materialSlot(0).name, "cloth");
	EXPECT_THROW((void)source.materialSlot(1), spk::Exception);
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
	arm.setGeometry(model("skin").geometry());
	arm.addAnchor("shoulder", Matrix::translation(1, 0, 0));
	Model::Builder torso;
	torso.addAnchor("leftArm");
	torso.attach("leftArm", arm.build(), "shoulder");
	const Model published = torso.build();
	EXPECT_EQ(published.anchor("leftArm").attachment(), "shoulder");
	ASSERT_NE(published.anchor("leftArm").model(), nullptr);
	EXPECT_THROW(torso.attach("leftArm", arm.build(), "missing"), spk::Exception);
	EXPECT_EQ(torso.build().anchor("leftArm").attachment(), "shoulder");
	torso.detach("leftArm");
	EXPECT_EQ(torso.build().anchor("leftArm").model(), nullptr);
	EXPECT_EQ(torso.build().anchor("leftArm").attachment(), Model::Anchor::NoAttachmentPoint);
	EXPECT_NE(published.anchor("leftArm").model(), nullptr);
}

TEST(VoxelModelTest, CopiesShareImmutableHierarchies)
{
	Model::Builder builder;
	builder.addAnchor("arm");
	builder.attach("arm", model("skin"));
	const Model original = builder.build();
	const Model copy = original;
	EXPECT_EQ(&original.anchors(), &copy.anchors());
	ASSERT_NE(copy.anchor("arm").model(), nullptr);
	EXPECT_EQ(&copy.anchor("arm").model()->opaqueMesh(), &original.anchor("arm").model()->opaqueMesh());
	builder.addAnchor("new");
	EXPECT_FALSE(copy.hasAnchor("new"));
	EXPECT_TRUE(builder.build().hasAnchor("new"));
}

TEST(VoxelModelViewTest, ConstructorCreatesAllAnchorViewsAndChildViews)
{
	Model::Builder arm;
	arm.addAnchor("elbow");
	Model::Builder torso;
	torso.addAnchor("hand");
	torso.addAnchor("shoulder");
	torso.attach("shoulder", arm.build());
	Model::View view(torso.build());
	EXPECT_FALSE(view.anchor("hand").hasChild());
	EXPECT_THROW((void)view.anchor("hand").child(), spk::Exception);
	ASSERT_TRUE(view.anchor("shoulder").hasChild());
	EXPECT_FALSE(view.anchor("shoulder").child().anchor("elbow").hasChild());
	EXPECT_EQ(view.anchor("shoulder").rotation(), spk::Quaternion::identity());
	EXPECT_EQ(view.anchor("shoulder").child().palette(), nullptr);
}

TEST(VoxelModelViewTest, ChildViewTransformComposesAfterJointAndAttachment)
{
	Model::Builder torso;
	torso.addAnchor("shoulder", Matrix::translation(2, 0, 0));
	Model::Builder arm;
	arm.addAnchor("origin", Matrix::translation(1, 0, 0));
	torso.attach("shoulder", arm.build(), "origin");
	Model::View view(torso.build());
	view.setTransform(Matrix::translation(5, 0, 0));
	view.anchor("shoulder").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	view.anchor("shoulder").child().setTransform(Matrix::translation(0, 0, 3));
	std::vector<Matrix> transforms;
	view.visit([&](const Model &, const spk::Palette &, const Matrix &matrix) {
		transforms.push_back(matrix);
	});
	ASSERT_EQ(transforms.size(), 2u);
	const auto parent = Matrix::translation(5, 0, 0) * Matrix::translation(2, 0, 0);
	const auto joint = Matrix::rotation(view.anchor("shoulder").rotation()) * Matrix::translation(-1, 0, 0);
	const auto expected = parent * joint * Matrix::translation(0, 0, 3);
	expectMatrix(transforms[1], expected);
}

TEST(VoxelModelViewTest, IndependentPalettesForEachModel)
{
	spk::Texture atlas;
	Model::Builder torsoBuilder(model("cloth"));
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.attach("leftArm", model("skin"));
	Model::View view(torsoBuilder.build());
	EXPECT_FALSE(view.isRenderable());
	view.bind(palette(atlas, {"cloth"}), Model::PalettePropagation::Self);
	view.anchor("leftArm").child().bind(palette(atlas, {"skin"}));
	EXPECT_TRUE(view.isRenderable());
	ASSERT_NE(view.palette(), nullptr);
	ASSERT_NE(view.anchor("leftArm").child().palette(), nullptr);
	EXPECT_NE(&view.palette()->materials(), &view.anchor("leftArm").child().palette()->materials());
}

TEST(VoxelModelViewTest, DefaultPaletteBindingOnlyChangesSelectedModel)
{
	spk::Texture atlas;
	Model::Builder builder(model("cloth"));
	builder.addAnchor("arm");
	builder.attach("arm", model("skin"));
	Model::View view(builder.build());
	const auto shared = palette(atlas, {"cloth", "skin"});
	view.bind(shared);
	ASSERT_NE(view.palette(), nullptr);
	EXPECT_EQ(view.anchor("arm").child().palette(), nullptr);
	EXPECT_FALSE(view.isRenderable());
	const auto childPalette = palette(atlas, {"skin"});
	view.anchor("arm").child().bind(childPalette);
	EXPECT_TRUE(view.isRenderable());
	const auto rootPalette = palette(atlas, {"cloth"});
	view.bind(rootPalette);
	EXPECT_EQ(&view.palette()->materials(), &rootPalette.materials());
	EXPECT_EQ(&view.anchor("arm").child().palette()->materials(), &childPalette.materials());
}

TEST(VoxelModelViewTest, RecursivePaletteBindingIsAtomic)
{
	spk::Texture atlas;
	Model::Builder torsoBuilder(model("cloth"));
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.attach("leftArm", model("skin"));
	Model::View view(torsoBuilder.build());
	const auto complete = palette(atlas, {"cloth", "skin"});
	view.bind(complete, Model::PalettePropagation::Recursive);
	EXPECT_TRUE(view.isRenderable());
	EXPECT_THROW(view.bind(palette(atlas, {"cloth"}), Model::PalettePropagation::Recursive), spk::Exception);
	EXPECT_EQ(&view.palette()->materials(), &complete.materials());
	EXPECT_EQ(&view.anchor("leftArm").child().palette()->materials(), &complete.materials());
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
	torsoBuilder.attach("shoulder", armBuilder.build(), "origin");
	Model::View view(torsoBuilder.build());
	view.anchor("shoulder").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	const auto rotation = Matrix::rotation(view.anchor("shoulder").rotation());
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
	torsoBuilder.attach("arm", Model{});
	Model::View first(torsoBuilder.build());
	Model::View second(torsoBuilder.build());
	first.anchor("arm").setRotation(spk::Quaternion::fromEuler({0, 0, 45}));
	EXPECT_NE(first.anchor("arm").rotation(), second.anchor("arm").rotation());
	EXPECT_EQ(second.anchor("arm").rotation(), spk::Quaternion::identity());
}

TEST(VoxelModelViewTest, DuplicateAnchorNamesRejectAmbiguousLookup)
{
	Model::Builder torsoBuilder;
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.addAnchor("rightArm");
	Model::Builder armBuilder;
	armBuilder.addAnchor("elbow");
	torsoBuilder.attach("leftArm", armBuilder.build());
	torsoBuilder.attach("rightArm", armBuilder.build());
	Model::View view(torsoBuilder.build());
	EXPECT_THROW(view.anchor("elbow").setRotation(spk::Quaternion::identity()), spk::Exception);
}

TEST(VoxelModelBuilderTest, HarmonizeCombinesSlotsAndPreservesSources)
{
	Model::Builder builder(model("skin"));
	builder.addAnchor("arm");
	builder.attach("arm", model("cloth"));
	const Model before = builder.build();
	builder.harmonize();
	const Model result = builder.build();
	EXPECT_EQ(result.materialSlots().size(), 2u);
	EXPECT_EQ(result.materialSlots()[0].name, "cloth");
	EXPECT_EQ(result.materialSlots()[1].name, "skin");
	EXPECT_EQ(materialIndex(result), 1u);
	ASSERT_NE(result.anchor("arm").model(), nullptr);
	EXPECT_EQ(materialIndex(*result.anchor("arm").model()), 0u);
	EXPECT_EQ(materialIndex(before), 0u);
	EXPECT_EQ(materialIndex(*before.anchor("arm").model()), 0u);
	spk::Texture atlas;
	Model::View view(result);
	view.bind(palette(atlas, {"cloth", "skin"}), Model::PalettePropagation::Recursive);
	EXPECT_TRUE(view.isRenderable());
}

TEST(VoxelModelBuilderTest, HarmonizeUnifiesSameNameAndOpacity)
{
	Model::Builder builder(model("cloth"));
	builder.addAnchor("left");
	builder.addAnchor("right");
	builder.attach("left", model("cloth"));
	builder.attach("right", model("cloth"));
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
	builder.attach("arm", Model(Geometry({}, {}, {}, {{"cloth", 0.5f}})));
	const Model before = builder.build();
	EXPECT_THROW(builder.harmonize(), spk::Exception);
	ASSERT_EQ(builder.build().materialSlots().size(), before.materialSlots().size());
	EXPECT_EQ(builder.build().materialSlots()[0].name, before.materialSlots()[0].name);
	EXPECT_EQ(materialIndex(builder.build()), 0u);
}

TEST(VoxelModelBuilderTest, HarmonizeAlwaysRecomputesEvenWhenAlreadyHarmonized)
{
	Model::Builder builder(model("cloth"));
	builder.addAnchor("arm");
	builder.attach("arm", model("skin"));
	builder.harmonize();
	const auto first = builder.build();
	builder.harmonize();
	const auto second = builder.build();
	EXPECT_EQ(first.materialSlots().size(), second.materialSlots().size());
	EXPECT_EQ(materialIndex(first), materialIndex(second));
	EXPECT_EQ(materialIndex(*first.anchor("arm").model()), materialIndex(*second.anchor("arm").model()));
	EXPECT_NE(&first.opaqueMesh().layout(), &second.opaqueMesh().layout());
	EXPECT_NE(&first.anchor("arm").model()->opaqueMesh().layout(), &second.anchor("arm").model()->opaqueMesh().layout());
}

TEST(VoxelModelBuilderTest, ReharmonizingAfterAddingChildUpdatesAllIndices)
{
	Model::Builder builder(model("skin"));
	builder.harmonize();
	const Model original = builder.build();
	EXPECT_EQ(materialIndex(original), 0u);
	builder.addAnchor("arm");
	builder.attach("arm", model("cloth"));
	builder.harmonize();
	const Model changed = builder.build();
	ASSERT_EQ(changed.materialSlots().size(), 2u);
	EXPECT_EQ(changed.materialSlots()[0].name, "cloth");
	EXPECT_EQ(changed.materialSlots()[1].name, "skin");
	EXPECT_EQ(materialIndex(changed), 1u);
	EXPECT_EQ(materialIndex(*changed.anchor("arm").model()), 0u);
	EXPECT_EQ(materialIndex(original), 0u);
}

TEST(VoxelModelBuilderTest, HarmonizedGeometryAcceptsPalettesWithAdditionalSlots)
{
	Model::Builder builder(model("skin"));
	builder.addAnchor("arm");
	builder.attach("arm", model("cloth"));
	builder.harmonize();
	const Model result = builder.build();
	spk::Texture atlas;
	const auto appearance = palette(atlas, {"cloth", "skin", "extra"});
	EXPECT_TRUE(result.isCompatible(appearance));
	EXPECT_TRUE(result.anchor("arm").model()->isCompatible(appearance));
	Model::View view(result);
	view.bind(appearance, Model::PalettePropagation::Recursive);
	EXPECT_TRUE(view.isRenderable());
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
	torsoBuilder.attach("leftShoulder", armBuilder.build());
	torsoBuilder.attach("rightShoulder", armBuilder.build());
	Model::View view(torsoBuilder.build());
	const auto left = Path{"leftShoulder", "elbow"};
	const auto right = Path{"rightShoulder", "elbow"};
	const auto rotation = spk::Quaternion::fromEuler({0, 0, 45});
	view.anchor(left).setRotation(rotation);
	EXPECT_EQ(view.anchor(left).rotation(), rotation.normalized());
	EXPECT_EQ(view.anchor(right).rotation(), spk::Quaternion::identity());
	EXPECT_THROW(view.anchor("elbow").setRotation(rotation), spk::Exception);
	EXPECT_THROW(view.anchor(Path{"leftShoulder", "missing"}).setRotation(rotation), spk::Exception);
}

TEST(VoxelModelViewTest, AnchorRotationsBelongToEachView)
{
	Model::Builder builder;
	builder.addAnchor("arm");
	builder.attach("arm", Model{});
	const Model source = builder.build();
	Model::View first(source);
	Model::View second(source);
	const auto rotation = spk::Quaternion::fromEuler({0, 0, 60});
	first.anchor("arm").setRotation(rotation);
	EXPECT_EQ(first.anchor("arm").rotation(), rotation.normalized());
	EXPECT_EQ(second.anchor("arm").rotation(), spk::Quaternion::identity());
	first.anchor("arm").resetRotation();
	EXPECT_EQ(first.anchor("arm").rotation(), spk::Quaternion::identity());
}

TEST(VoxelModelViewTest, ShortNameLookupRequiresUniqueness)
{
	Model::Builder torsoBuilder;
	Model::Builder armBuilder;
	armBuilder.addAnchor("elbow");
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.attach("leftArm", armBuilder.build());
	Model::View single(torsoBuilder.build());
	EXPECT_NO_THROW((void)single.anchor("elbow"));
	torsoBuilder.addAnchor("rightArm");
	torsoBuilder.attach("rightArm", armBuilder.build());
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
	torsoBuilder.attach("leftArm", armBuilder.build());
	Model::View view(torsoBuilder.build());
	const auto rotation = spk::Quaternion::fromEuler({0, 0, 33});
	view.anchor(Path{"leftArm/elbow"}).setRotation(rotation);
	EXPECT_EQ(view.anchor(Path{"leftArm/elbow"}).rotation(), rotation.normalized());
	EXPECT_EQ(view.anchor(Path{"leftArm", "elbow"}).rotation(), spk::Quaternion::identity());
}

TEST(VoxelModelViewTest, TypedPathsApplyIndependentChildPalettes)
{
	using Path = Model::Anchor::Path;
	spk::Texture atlas;
	Model::Builder armBuilder(model("skin"));
	armBuilder.addAnchor("elbow");
	armBuilder.attach("elbow", model("metal"));
	Model::Builder torsoBuilder;
	torsoBuilder.addAnchor("leftArm");
	torsoBuilder.addAnchor("rightArm");
	torsoBuilder.attach("leftArm", armBuilder.build());
	torsoBuilder.attach("rightArm", armBuilder.build());
	Model::View view(torsoBuilder.build());
	const auto skin = palette(atlas, {"skin"});
	const auto metal = palette(atlas, {"metal"});
	view.anchor(Path{"leftArm"}).child().bind(skin);
	view.anchor(Path{"rightArm"}).child().bind(skin);
	view.anchor(Path{"leftArm", "elbow"}).child().bind(metal);
	view.anchor(Path{"rightArm", "elbow"}).child().bind(metal);
	ASSERT_NE(view.anchor(Path{"leftArm", "elbow"}).child().palette(), nullptr);
	EXPECT_EQ(&view.anchor(Path{"leftArm", "elbow"}).child().palette()->materials(), &metal.materials());
	EXPECT_TRUE(view.isRenderable());
}

TEST(VoxelModelViewTest, MovePreservesChildViewState)
{
	spk::Texture atlas;
	Model::Builder builder;
	builder.addAnchor("arm");
	builder.attach("arm", model("skin"));
	Model::View original(builder.build());
	const auto childPalette = palette(atlas, {"skin"});
	original.anchor("arm").child().bind(childPalette);
	original.anchor("arm").setRotation(spk::Quaternion::fromEuler({0, 0, 45}));
	Model::View moved(std::move(original));
	ASSERT_TRUE(moved.anchor("arm").hasChild());
	ASSERT_NE(moved.anchor("arm").child().palette(), nullptr);
	EXPECT_EQ(&moved.anchor("arm").child().palette()->materials(), &childPalette.materials());
	EXPECT_EQ(moved.anchor("arm").rotation(), spk::Quaternion::fromEuler({0, 0, 45}).normalized());
}

TEST(VoxelModelViewTest, CopiedViewsOwnIndependentChildStates)
{
	spk::Texture atlas;
	Model::Builder builder(model("cloth"));
	builder.addAnchor("arm");
	builder.attach("arm", model("skin"));
	Model::View original(builder.build());
	original.bind(palette(atlas, {"cloth", "skin"}), Model::PalettePropagation::Recursive);
	Model::View copy(original);
	const auto rotation = spk::Quaternion::fromEuler({0, 0, 35});
	copy.anchor("arm").setRotation(rotation);
	copy.anchor("arm").child().bind(palette(atlas, {"skin", "other"}));
	EXPECT_EQ(original.anchor("arm").rotation(), spk::Quaternion::identity());
	EXPECT_NE(&original.anchor("arm").child().model(), &copy.anchor("arm").child().model());
	EXPECT_EQ(&original.anchor("arm").child().model().geometry(), &copy.anchor("arm").child().model().geometry());
	EXPECT_NE(&original.anchor("arm").child().palette()->materials(), &copy.anchor("arm").child().palette()->materials());
	copy = original;
	EXPECT_EQ(copy.anchor("arm").rotation(), spk::Quaternion::identity());
	EXPECT_EQ(&copy.anchor("arm").child().palette()->materials(), &original.anchor("arm").child().palette()->materials());
}
