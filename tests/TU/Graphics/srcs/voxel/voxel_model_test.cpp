#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "exception.hpp"
#include "model.hpp"
#include "voxel/model_view.hpp"

namespace
{
	using Model = spk::Model;
	using Skeleton = Model::Skeleton;
	using Part = Skeleton::Part;
	using Matrix = spk::Matrix4x4;

	[[nodiscard]] spk::Voxel::Volume volume(const std::string &slot)
	{
		spk::Voxel::Shape::Builder shape;
		shape.addFromJSON(spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / "cube.json"));
		const auto cube = shape.build();
		spk::Voxel::Definition::SlotCollection slots;
		for (const auto &entry : cube.slots())
		{
			slots.emplace(entry.name, spk::Voxel::Definition::Slot{slot, 1.0f});
		}
		spk::Voxel::Definition::Collection::Builder definitions;
		definitions.add(1, spk::Voxel::Definition(cube, std::move(slots)));
		spk::Voxel::Volume::Builder builder({1, 1, 1});
		builder.bind(definitions.build());
		builder.set({0, 0, 0}, 1);
		return builder.build();
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
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				EXPECT_NEAR(actual[column][row], expected[column][row], 0.0001f);
			}
		}
	}
}

TEST(ModelGraphicsTest, PartsWithoutVolumeNeedNoMeshOrPalette)
{
	Model model;
	model.skeleton().root().addJoint("hand");
	model.skeleton().joint("hand").attach(Part{});
	Model::View view(model);
	EXPECT_EQ(view.root().geometry(), nullptr);
	EXPECT_EQ(view.part("hand").geometry(), nullptr);
	EXPECT_TRUE(view.isRenderable());
	std::size_t count = 0;
	view.visit([&](const Part &, const spk::Voxel::Geometry &, const spk::Palette &, const Matrix &) {
		++count;
	});
	EXPECT_EQ(count, 0u);
}

TEST(ModelGraphicsTest, ViewBuildsAndCachesRenderMeshesOnDemand)
{
	Model model;
	model.skeleton().root().setVolume(volume("stone"));
	Model::View view(model);
	ASSERT_NE(view.root().geometry(), nullptr);
	const auto *first = view.root().geometry();
	EXPECT_EQ(first, view.root().geometry());
	EXPECT_GT(first->opaqueMesh().indexCount(), 0u);
	EXPECT_GT(first->depthMesh().indexCount(), 0u);
	EXPECT_EQ(first->materialSlots()[0].name, "stone");
	model.skeleton().root().setVolume(volume("sand"));
	const auto *second = view.root().geometry();
	EXPECT_EQ(first, second);
	EXPECT_EQ(second->materialSlots()[0].name, "sand");
	EXPECT_GT(second->depthMesh().indexCount(), 0u);
}

TEST(ModelGraphicsTest, CoreJointsProvidePoseAndGraphicsOnlyProvidePalette)
{
	Model model;
	model.skeleton().root().setVolume(volume("cloth"));
	model.skeleton().root().addJoint("arm", Matrix::translation(2, 0, 0));
	Part arm(volume("skin"));
	arm.addJoint("pivot", Matrix::translation(1, 0, 0));
	model.skeleton().joint("arm").attach(arm, "pivot");
	model.setTransform(Matrix::translation(5, 0, 0));
	Model::View view(model);
	spk::Texture atlas;
	view.root().bind(palette(atlas, {"cloth"}));
	view.part("arm").bind(palette(atlas, {"skin"}));
	EXPECT_TRUE(view.isRenderable());
	model.skeleton().joint("arm").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	std::vector<Matrix> transforms;
	view.visit([&](const Part &, const spk::Voxel::Geometry &, const spk::Palette &, const Matrix &matrix) {
		transforms.push_back(matrix);
	});
	ASSERT_EQ(transforms.size(), 2u);
	expectMatrix(transforms[0], Matrix::translation(5, 0, 0));
	const auto rotation = Matrix::rotation(model.skeleton().joint("arm").rotation());
	const auto jointTransform = Matrix::translation(5, 0, 0) * Matrix::translation(2, 0, 0);
	expectMatrix(transforms[1], jointTransform * rotation * Matrix::translation(-1, 0, 0));
}

TEST(ModelGraphicsTest, BindingOnePartDoesNotBindItsChildren)
{
	Model model;
	model.skeleton().root().setVolume(volume("cloth"));
	model.skeleton().root().addJoint("arm");
	model.skeleton().joint("arm").attach(Part(volume("skin")));
	Model::View view(model);
	spk::Texture atlas;
	view.bind(palette(atlas, {"cloth", "skin"}));
	EXPECT_EQ(view.part("arm").palette(), nullptr);
	EXPECT_FALSE(view.isRenderable());
	view.part("arm").bind(palette(atlas, {"skin"}));
	EXPECT_TRUE(view.isRenderable());
}

TEST(ModelGraphicsTest, RecursiveBindingRejectsMissingMaterialsWithoutChangingPalettes)
{
	Model model;
	model.skeleton().root().setVolume(volume("cloth"));
	model.skeleton().root().addJoint("arm");
	model.skeleton().joint("arm").attach(Part(volume("skin")));
	Model::View view(model);
	spk::Texture atlas;
	const auto complete = palette(atlas, {"cloth", "skin"});
	view.bind(complete, Model::View::PalettePropagation::Recursive);
	EXPECT_TRUE(view.isRenderable());
	EXPECT_THROW(view.bind(palette(atlas, {"cloth"}), Model::View::PalettePropagation::Recursive), spk::Exception);
	ASSERT_NE(view.root().palette(), nullptr);
	ASSERT_NE(view.part("arm").palette(), nullptr);
	EXPECT_EQ(&view.root().palette()->materials(), &complete.materials());
	EXPECT_EQ(&view.part("arm").palette()->materials(), &complete.materials());
}

TEST(ModelGraphicsTest, TwoViewsObserveTheSameCorePoseButOwnIndependentPalettes)
{
	Model model;
	model.skeleton().root().setVolume(volume("skin"));
	model.skeleton().root().addJoint("arm");
	model.skeleton().joint("arm").attach(Part(volume("skin")));
	Model::View first(model);
	Model::View second(model);
	spk::Texture a;
	spk::Texture b;
	first.bind(palette(a, {"skin"}), Model::View::PalettePropagation::Recursive);
	second.bind(palette(b, {"skin"}), Model::View::PalettePropagation::Recursive);
	model.skeleton().joint("arm").setRotation(spk::Quaternion::fromEuler({0, 0, 30}));
	std::vector<Matrix> firstMatrices;
	std::vector<Matrix> secondMatrices;
	first.visit([&](const Part &, const spk::Voxel::Geometry &, const spk::Palette &, const Matrix &m) {
		firstMatrices.push_back(m);
	});
	second.visit([&](const Part &, const spk::Voxel::Geometry &, const spk::Palette &, const Matrix &m) {
		secondMatrices.push_back(m);
	});
	ASSERT_EQ(firstMatrices.size(), 2u);
	ASSERT_EQ(secondMatrices.size(), 2u);
	expectMatrix(firstMatrices[1], secondMatrices[1]);
	EXPECT_NE(&first.root().palette()->materials(), &second.root().palette()->materials());
}

TEST(ModelGraphicsTest, SkeletonTopologyChangeRequiresRecreatingTheView)
{
	Model model;
	Model::View view(model);
	model.skeleton().root().addJoint("new");
	EXPECT_THROW((void)view.root().geometry(), spk::Exception);
	EXPECT_THROW((void)view.isRenderable(), spk::Exception);
	Model::View rebuilt(model);
	EXPECT_TRUE(rebuilt.isRenderable());
	EXPECT_THROW((void)rebuilt.part("new"), spk::Exception);
}

TEST(ModelGraphicsTest, ViewsCanFindUniquePartsOrTypedPaths)
{
	Model model;
	model.skeleton().root().addJoint("left");
	model.skeleton().root().addJoint("right");
	Part arm;
	arm.addJoint("elbow");
	arm.joint("elbow").attach(Part{});
	model.skeleton().root().joint("left").attach(arm);
	model.skeleton().root().joint("right").attach(arm);
	Model::View view(model);
	EXPECT_THROW((void)view.part("elbow"), spk::Exception);
	EXPECT_NO_THROW((void)view.part(Model::View::Path{"left", "elbow"}));
	EXPECT_THROW((void)view.part(Model::View::Path{"left", "missing"}), spk::Exception);
}

TEST(ModelGraphicsTest, ViewCopiesHaveIndependentGraphicCaches)
{
	Model model;
	model.skeleton().root().setVolume(volume("stone"));
	Model::View first(model);
	Model::View second(first);
	spk::Texture atlas;
	first.bind(palette(atlas, {"stone"}));
	EXPECT_EQ(second.root().palette(), nullptr);
	EXPECT_EQ(&first.root().part(), &second.root().part());
	ASSERT_NE(first.root().geometry(), nullptr);
	ASSERT_NE(second.root().geometry(), nullptr);
	EXPECT_NE(first.root().geometry(), second.root().geometry());
}

TEST(ModelGraphicsTest, HarmonizeRemapsOnlyGraphicalMeshes)
{
	Model model;
	model.skeleton().root().setVolume(volume("skin"));
	model.skeleton().root().addJoint("arm");
	model.skeleton().joint("arm").attach(Part(volume("cloth")));
	Model::View view(model);
	ASSERT_NE(view.root().geometry(), nullptr);
	const auto &buffer = view.root().geometry()->opaqueMesh().layout().vertexBuffer();
	const auto original = buffer.cast<spk::Voxel::Geometry::Vertex>()[0].materialIndex;
	EXPECT_EQ(original, 0u);
	view.harmonize();
	const auto *body = view.root().geometry();
	const auto *arm = view.part("arm").geometry();
	ASSERT_NE(body, nullptr);
	ASSERT_NE(arm, nullptr);
	EXPECT_EQ(body->materialSlots().size(), 2u);
	EXPECT_EQ(arm->materialSlots().size(), 2u);
	EXPECT_EQ(body->materialSlots()[0].name, "cloth");
	EXPECT_EQ(body->materialSlots()[1].name, "skin");
	EXPECT_EQ(body->opaqueMesh().layout().vertexBuffer().cast<spk::Voxel::Geometry::Vertex>()[0].materialIndex, 1u);
	EXPECT_EQ(arm->opaqueMesh().layout().vertexBuffer().cast<spk::Voxel::Geometry::Vertex>()[0].materialIndex, 0u);
	Model::View untouched(model);
	EXPECT_EQ(untouched.root().geometry()->materialSlots().size(), 1u);
	spk::Texture atlas;
	view.bind(palette(atlas, {"skin", "cloth"}), Model::View::PalettePropagation::Recursive);
	EXPECT_TRUE(view.isRenderable());
}
