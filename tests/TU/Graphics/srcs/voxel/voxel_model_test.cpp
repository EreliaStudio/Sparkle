#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "exception.hpp"
#include "voxel/model.hpp"
#include "voxel/mesher.hpp"
#include "voxel/palette.hpp"

namespace
{
	using Model = spk::Voxel::Model;
	using SurfaceMesh = spk::Voxel::SurfaceMesh;
	using Mesh = spk::Voxel::Mesh;
	using Vertex = spk::Voxel::MeshVertex;
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

TEST(VoxelModelTest, AddingRemovingAndEditingAnchors)
{
	Model torso;
	EXPECT_FALSE(torso.hasAnchor("shoulder"));
	torso.addAnchor("shoulder", Matrix::translation(2, 0, 0));
	EXPECT_TRUE(torso.hasAnchor("shoulder"));
	expectMatrix(torso.anchor("shoulder").transform(), Matrix::translation(2, 0, 0));
	torso.anchor("shoulder").setTransform(Matrix::translation(3, 0, 0));
	expectMatrix(torso.anchor("shoulder").transform(), Matrix::translation(3, 0, 0));
	EXPECT_THROW(torso.addAnchor("shoulder"), spk::Exception);
	EXPECT_THROW((void)torso.anchor("unknown"), spk::Exception);
	torso.removeAnchor("shoulder");
	EXPECT_FALSE(torso.hasAnchor("shoulder"));
	EXPECT_THROW(torso.removeAnchor("shoulder"), spk::Exception);
}

TEST(VoxelModelTest, AnchorsRequireRigidFiniteMatrices)
{
	Model torso;
	const auto invalid = Matrix::scale(2, 1, 1);
	EXPECT_THROW(torso.addAnchor("bad", invalid), spk::Exception);
	torso.addAnchor("good");
	EXPECT_THROW(torso.anchor("good").setTransform(invalid), spk::Exception);
	expectMatrix(torso.anchor("good").transform(), Matrix::identity());
}

TEST(VoxelModelTest, BindClonesHierarchyAndChecksChildAttachment)
{
	Model torso;
	torso.addAnchor("leftArm");
	Model arm = model("skin");
	arm.addAnchor("shoulder", Matrix::translation(1, 0, 0));
	torso.anchor("leftArm").bind(arm, "shoulder");
	EXPECT_EQ(torso.anchor("leftArm").attachment(), "shoulder");
	ASSERT_NE(torso.anchor("leftArm").model(), nullptr);
	EXPECT_EQ(torso.anchor("leftArm").model()->materialSlots()[0].name, "skin");
	EXPECT_THROW(torso.anchor("leftArm").bind(arm, "missing"), spk::Exception);
	EXPECT_NE(torso.anchor("leftArm").model(), nullptr);
	torso.anchor("leftArm").unbind();
	EXPECT_EQ(torso.anchor("leftArm").model(), nullptr);
	EXPECT_EQ(torso.anchor("leftArm").attachment(), std::nullopt);
}

TEST(VoxelModelTest, CopiesDoNotShareMutableHierarchy)
{
	Model torso;
	torso.addAnchor("leftArm");
	torso.anchor("leftArm").bind(model("skin"));
	Model copy = torso;
	ASSERT_NE(copy.anchor("leftArm").model(), nullptr);
	EXPECT_EQ(&copy.anchor("leftArm").model()->opaqueMesh().layout(),
		&torso.anchor("leftArm").model()->opaqueMesh().layout());
	copy.anchor("leftArm").model()->addAnchor("elbow");
	EXPECT_FALSE(torso.anchor("leftArm").model()->hasAnchor("elbow"));
	EXPECT_TRUE(copy.anchor("leftArm").model()->hasAnchor("elbow"));
}

TEST(VoxelModelViewTest, IndependentPalettesForEachModel)
{
	spk::Texture atlas;
	Model torso = model("cloth");
	torso.addAnchor("leftArm");
	torso.anchor("leftArm").bind(model("skin"));
	Model::View view(torso);
	EXPECT_FALSE(view.isRenderable());
	view.setPalette(palette(atlas, {"cloth"}), spk::Voxel::PalettePropagation::Self);
	view.setPalette("leftArm", palette(atlas, {"skin"}));
	EXPECT_TRUE(view.isRenderable());
	ASSERT_NE(view.palette(), nullptr);
	ASSERT_NE(view.palette("leftArm"), nullptr);
	EXPECT_NE(&view.palette()->materials(), &view.palette("leftArm")->materials());
}

TEST(VoxelModelViewTest, RecursivePaletteBindingIsAtomic)
{
	spk::Texture atlas;
	Model torso = model("cloth");
	torso.addAnchor("leftArm");
	torso.anchor("leftArm").bind(model("skin"));
	Model::View view(torso);
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
	EXPECT_NE(red.palette()->find("stone")->find("albedo")->texture.identifier(),
		blue.palette()->find("stone")->find("albedo")->texture.identifier());
}

TEST(VoxelModelViewTest, ChildJointRotationKeepsAttachmentAligned)
{
	Model torso;
	torso.addAnchor("shoulder", Matrix::translation(2, 0, 0));
	Model arm;
	arm.addAnchor("origin", Matrix::translation(1, 0, 0));
	torso.anchor("shoulder").bind(arm, "origin");
	Model::View view(torso);
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
	Model torso;
	torso.addAnchor("arm");
	torso.anchor("arm").bind(Model{});
	Model::View first(torso);
	Model::View second(torso);
	first.setAnchorRotation("arm", spk::Quaternion::fromEuler({0, 0, 45}));
	EXPECT_NE(first.anchorRotation("arm"), second.anchorRotation("arm"));
	EXPECT_EQ(second.anchorRotation("arm"), spk::Quaternion::identity());
}

TEST(VoxelModelViewTest, DuplicateAnchorNamesRejectAmbiguousLookup)
{
	Model torso;
	torso.addAnchor("leftArm");
	torso.addAnchor("rightArm");
	Model arm;
	arm.addAnchor("elbow");
	torso.anchor("leftArm").bind(arm);
	torso.anchor("rightArm").bind(arm);
	Model::View view(torso);
	EXPECT_THROW(view.setAnchorRotation("elbow", spk::Quaternion::identity()), spk::Exception);
}

TEST(VoxelModelHarmonizerTest, CombinesIndependentIndicesAndPreservesSource)
{
	spk::Texture atlas;
	Model torso = model("cloth", 5);
	torso.addAnchor("arm");
	torso.anchor("arm").bind(model("skin", 0));
	Model::View view(torso);
	view.setPalette(palette(atlas, {"cloth"}), spk::Voxel::PalettePropagation::Self);
	view.setPalette("arm", palette(atlas, {"skin"}));
	Model::Harmonizer harmonizer;
	const auto result = harmonizer.process(view);
	EXPECT_EQ(result.materialLayout.names(), (std::vector<std::string>{"cloth", "skin"}));
	EXPECT_TRUE(result.materialLayout.isCompatible(result.palette));
	EXPECT_EQ(materialIndex(result.model), 0u);
	ASSERT_NE(result.model.anchor("arm").model(), nullptr);
	EXPECT_EQ(materialIndex(*result.model.anchor("arm").model()), 1u);
	EXPECT_EQ(materialIndex(torso), 5u);
	EXPECT_EQ(materialIndex(*torso.anchor("arm").model()), 0u);
	Model::View unified(result.model, result.palette);
	EXPECT_TRUE(unified.isRenderable());
}

TEST(VoxelModelHarmonizerTest, HarmonizedGeometryAcceptsOtherMatchingPalettes)
{
	spk::Texture first;
	spk::Texture second;
	Model torso = model("cloth");
	torso.addAnchor("arm");
	torso.anchor("arm").bind(model("skin"));
	Model::View source(torso, palette(first, {"cloth", "skin"}));
	const auto result = Model::Harmonizer().process(source);
	Model::View other(result.model, palette(second, {"cloth", "skin"}));
	EXPECT_TRUE(other.isRenderable());
	EXPECT_THROW(other.setPalette(palette(second, {"cloth", "skin", "extra"})), spk::Exception);
	EXPECT_TRUE(other.isRenderable());
}

TEST(VoxelModelHarmonizerTest, SameSlotAndDifferentMaterialsRemainDistinct)
{
	spk::Texture atlas;
	spk::Material::Builder firstBuilder;
	spk::Material::Builder secondBuilder;
	firstBuilder.add("albedo", atlas, {{0, 0}, {0.5f, 1}});
	secondBuilder.add("albedo", atlas, {{0.5f, 0}, {0.5f, 1}});
	spk::Voxel::Palette::Builder first;
	spk::Voxel::Palette::Builder second;
	first.setMaterial("cloth", firstBuilder.build());
	second.setMaterial("cloth", secondBuilder.build());
	Model torso = model("cloth");
	torso.addAnchor("arm");
	torso.anchor("arm").bind(model("cloth"));
	Model::View source(torso);
	source.setPalette(first.build(), spk::Voxel::PalettePropagation::Self);
	source.setPalette("arm", second.build());
	const auto result = Model::Harmonizer().process(source);
	EXPECT_EQ(result.materialLayout.names().size(), 2u);
	EXPECT_NE(materialIndex(result.model), materialIndex(*result.model.anchor("arm").model()));
	EXPECT_TRUE(result.materialLayout.isCompatible(result.palette));
}

TEST(VoxelModelHarmonizerTest, RejectsDifferentAtlasesWithoutModifyingSource)
{
	spk::Texture first;
	spk::Texture second;
	Model torso = model("cloth");
	torso.addAnchor("arm");
	torso.anchor("arm").bind(model("skin"));
	Model::View view(torso);
	view.setPalette(palette(first, {"cloth"}), spk::Voxel::PalettePropagation::Self);
	view.setPalette("arm", palette(second, {"skin"}));
	EXPECT_THROW((void)Model::Harmonizer().process(view), spk::Exception);
	EXPECT_EQ(materialIndex(view.model()), 0u);
	EXPECT_TRUE(view.isRenderable());
}

TEST(VoxelModelHarmonizerTest, MissingChildPaletteIsRejected)
{
	spk::Texture atlas;
	Model torso = model("cloth");
	torso.addAnchor("arm");
	torso.anchor("arm").bind(model("skin"));
	Model::View view(torso);
	view.setPalette(palette(atlas, {"cloth"}), spk::Voxel::PalettePropagation::Self);
	EXPECT_THROW((void)Model::Harmonizer().process(view), spk::Exception);
}

TEST(VoxelMaterialLayoutTest, RequiresSortedUniqueNames)
{
	EXPECT_THROW((void)spk::Voxel::MaterialLayout({"skin", "cloth"}), spk::Exception);
	EXPECT_THROW((void)spk::Voxel::MaterialLayout({"cloth", "cloth"}), spk::Exception);
	spk::Voxel::MaterialLayout layout({"cloth", "skin"});
	EXPECT_EQ(layout.names().size(), 2u);
}
