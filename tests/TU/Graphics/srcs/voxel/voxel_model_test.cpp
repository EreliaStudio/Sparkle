#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "exception.hpp"
#include "model.hpp"
#include "voxel/model_view.hpp"
#include "voxel/volume.hpp"

namespace
{
	using Model = spk::Model;
	using Skeleton = spk::Skeleton;
	using Part = Skeleton::Part;
	using Matrix = spk::Matrix4x4;

	[[nodiscard]] const spk::Voxel::Volume &volume(const std::string &slot, float opacity = 1.0f)
	{
		using Key = std::pair<std::string, float>;
		static std::map<Key, spk::Voxel::Volume> volumes;
		const Key key{slot, opacity};
		if (const auto it = volumes.find(key); it != volumes.end())
		{
			return it->second;
		}
		spk::Voxel::Shape::Builder shape;
		shape.addFromJSON(spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / "cube.json"));
		const auto cube = shape.build();
		spk::Voxel::Definition::SlotCollection slots;
		for (const auto &entry : cube.slots())
		{
			slots.emplace(entry.name, spk::Voxel::Definition::Slot{slot, opacity});
		}
		spk::Voxel::Definition::Collection::Builder definitions;
		definitions.add(1, spk::Voxel::Definition(cube, std::move(slots)));
		spk::Voxel::Volume::Builder builder({1, 1, 1});
		builder.bind(definitions.build());
		builder.set({0, 0, 0}, 1);
		return volumes.emplace(key, builder.build()).first->second;
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
}

TEST(ModelGraphicsTest, ViewBuildsAndCachesRenderMeshesOnDemand)
{
	Model model;
	model.skeleton().root().setBody(&volume("stone"));
	Model::View view(model);
	ASSERT_NE(view.root().geometry(), nullptr);
	const auto *first = view.root().geometry();
	EXPECT_EQ(first, view.root().geometry());
	EXPECT_GT(first->opaqueMesh().indexCount(), 0u);
	EXPECT_GT(first->depthMesh().indexCount(), 0u);
	EXPECT_EQ(first->materialSlots()[0].name, "stone");
	model.skeleton().root().setBody(&volume("sand"));
	const auto *second = view.root().geometry();
	EXPECT_EQ(first, second);
	EXPECT_EQ(second->materialSlots()[0].name, "sand");
	EXPECT_GT(second->depthMesh().indexCount(), 0u);
}

TEST(ModelGraphicsTest, CoreJointsProvidePoseAndGraphicsOnlyProvidePalette)
{
	Model model;
	model.skeleton().root().setBody(&volume("cloth"));
	model.skeleton().root().addJoint("arm", Matrix::translation(2, 0, 0));
	Part arm(&volume("skin"));
	arm.addJoint("pivot", Matrix::translation(1, 0, 0));
	model.skeleton().joint("arm").attach(arm, "pivot");
	model.setTransform(Matrix::translation(5, 0, 0));
	Model::View view(model);
	spk::Texture atlas;
	view.bind(palette(atlas, {"cloth", "skin"}));
	ASSERT_TRUE(view.isRenderable());
	const auto *rootGeometry = view.root().geometry();
	const auto *armGeometry = view.part("arm").geometry();
	const auto before = model.version().pose;
	model.skeleton().joint("arm").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	EXPECT_GT(model.version().pose, before);
	EXPECT_EQ(model.transform()[3][0], 5.0f);
	EXPECT_EQ(model.skeleton().joint("arm").attachment(), "pivot");
	EXPECT_EQ(view.root().geometry(), rootGeometry);
	EXPECT_EQ(view.part("arm").geometry(), armGeometry);
	EXPECT_TRUE(view.isRenderable());
}

TEST(ModelGraphicsTest, SinglePaletteCoversEveryPart)
{
	Model model;
	model.skeleton().root().setBody(&volume("cloth"));
	model.skeleton().root().addJoint("arm");
	model.skeleton().joint("arm").attach(Part(&volume("skin")));
	Model::View view(model);
	spk::Texture atlas;
	EXPECT_FALSE(view.isRenderable());
	view.bind(palette(atlas, {"cloth", "skin"}));
	ASSERT_NE(view.palette(), nullptr);
	ASSERT_NE(view.part("arm").geometry(), nullptr);
	EXPECT_TRUE(view.isRenderable());
}

TEST(ModelGraphicsTest, PaletteBindingValidatesAllPartsWithoutPartialChanges)
{
	Model model;
	model.skeleton().root().setBody(&volume("cloth"));
	model.skeleton().root().addJoint("arm");
	model.skeleton().joint("arm").attach(Part(&volume("skin")));
	Model::View view(model);
	spk::Texture atlas;
	const auto complete = palette(atlas, {"cloth", "skin"});
	view.bind(complete);
	EXPECT_TRUE(view.isRenderable());
	EXPECT_THROW(view.bind(palette(atlas, {"cloth"})), spk::Exception);
	ASSERT_NE(view.palette(), nullptr);
	EXPECT_EQ(&view.palette()->materials(), &complete.materials());
	EXPECT_TRUE(view.isRenderable());
}

TEST(ModelGraphicsTest, TwoViewsObserveTheSameCorePoseButOwnIndependentPalettes)
{
	Model model;
	model.skeleton().root().setBody(&volume("skin"));
	model.skeleton().root().addJoint("arm");
	model.skeleton().joint("arm").attach(Part(&volume("skin")));
	Model::View first(model);
	Model::View second(model);
	spk::Texture a;
	spk::Texture b;
	first.bind(palette(a, {"skin"}));
	second.bind(palette(b, {"skin"}));
	const auto *firstGeometry = first.part("arm").geometry();
	const auto *secondGeometry = second.part("arm").geometry();
	const auto before = model.version().pose;
	model.skeleton().joint("arm").setRotation(spk::Quaternion::fromEuler({0, 0, 30}));
	EXPECT_GT(model.version().pose, before);
	EXPECT_EQ(&first.part("arm").part(), &second.part("arm").part());
	EXPECT_EQ(first.part("arm").geometry(), firstGeometry);
	EXPECT_EQ(second.part("arm").geometry(), secondGeometry);
	EXPECT_NE(firstGeometry, secondGeometry);
	ASSERT_NE(first.palette(), nullptr);
	ASSERT_NE(second.palette(), nullptr);
	EXPECT_NE(&first.palette()->materials(), &second.palette()->materials());
}

TEST(ModelGraphicsTest, TopologyChangesRefreshOnlyAffectedViews)
{
	Model model;
	model.skeleton().root().setBody(&volume("stone"));
	Model::View view(model);
	spk::Texture atlas;
	view.bind(palette(atlas, {"stone"}));
	const auto *cached = view.root().geometry();
	model.skeleton().root().addJoint("new");
	EXPECT_EQ(view.root().geometry(), cached);
	EXPECT_TRUE(view.isRenderable());
	EXPECT_THROW((void)view.part("new"), spk::Exception);
	model.skeleton().joint("new").attach(Part(&volume("wood")));
	EXPECT_NO_THROW((void)view.part("new"));
	EXPECT_FALSE(view.isRenderable());
	view.bind(palette(atlas, {"stone", "wood"}));
	EXPECT_TRUE(view.isRenderable());
	model.skeleton().joint("new").detach();
	EXPECT_THROW((void)view.part("new"), spk::Exception);
	EXPECT_TRUE(view.isRenderable());
	EXPECT_EQ(view.root().geometry(), cached);
	EXPECT_NE(view.palette(), nullptr);
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
	model.skeleton().root().setBody(&volume("stone"));
	Model::View first(model);
	Model::View second(first);
	spk::Texture atlas;
	first.bind(palette(atlas, {"stone"}));
	EXPECT_EQ(second.palette(), nullptr);
	EXPECT_EQ(&first.root().part(), &second.root().part());
	ASSERT_NE(first.root().geometry(), nullptr);
	ASSERT_NE(second.root().geometry(), nullptr);
	EXPECT_NE(first.root().geometry(), second.root().geometry());
}

TEST(ModelGraphicsTest, DifferentBodiesKeepTheirLocalMaterialIndices)
{
	Model model;
	model.skeleton().root().setBody(&volume("skin"));
	model.skeleton().root().addJoint("arm");
	model.skeleton().joint("arm").attach(Part(&volume("cloth")));
	Model::View view(model);
	const auto *skin = view.root().geometry();
	const auto *cloth = view.part("arm").geometry();
	ASSERT_NE(skin, nullptr);
	ASSERT_NE(cloth, nullptr);
	ASSERT_EQ(skin->materialSlots().size(), 1u);
	ASSERT_EQ(cloth->materialSlots().size(), 1u);
	EXPECT_EQ(skin->materialSlots()[0].name, "skin");
	EXPECT_EQ(cloth->materialSlots()[0].name, "cloth");
	EXPECT_EQ(skin->opaqueMesh().layout().vertexBuffer().cast<spk::Voxel::Geometry::Vertex>()[0].materialIndex, 0u);
	EXPECT_EQ(cloth->opaqueMesh().layout().vertexBuffer().cast<spk::Voxel::Geometry::Vertex>()[0].materialIndex, 0u);
	spk::Texture atlas;
	view.bind(palette(atlas, {"cloth", "skin"}));
	EXPECT_TRUE(view.isRenderable());
}

TEST(ModelGraphicsTest, IndependentBodiesRetainTheirMaterialOpacities)
{
	Model model;
	model.skeleton().root().setBody(&volume("shared", 1.0f));
	model.skeleton().root().addJoint("overlay");
	model.skeleton().joint("overlay").attach(Part(&volume("shared", 0.35f)));
	const auto before = model.version();
	Model::View view(model);
	const auto *opaque = view.root().geometry();
	const auto *translucent = view.part("overlay").geometry();
	ASSERT_NE(opaque, nullptr);
	ASSERT_NE(translucent, nullptr);
	ASSERT_EQ(opaque->materialSlots().size(), 1u);
	ASSERT_EQ(translucent->materialSlots().size(), 1u);
	EXPECT_FLOAT_EQ(opaque->materialSlots()[0].opacity, 1.0f);
	EXPECT_FLOAT_EQ(translucent->materialSlots()[0].opacity, 0.35f);
	EXPECT_EQ(opaque->opaqueMesh().layout().vertexBuffer().cast<spk::Voxel::Geometry::Vertex>()[0].materialIndex, 0u);
	EXPECT_EQ(translucent->transparentMesh().layout().vertexBuffer().cast<spk::Voxel::Geometry::Vertex>()[0].materialIndex, 0u);
	EXPECT_EQ(model.version(), before);
	spk::Texture atlas;
	view.bind(palette(atlas, {"shared"}));
	EXPECT_TRUE(view.isRenderable());
}

TEST(ModelGraphicsTest, AttachedModelPoseChangesPreserveGraphicCaches)
{
	Model parent;
	parent.skeleton().root().addJoint("hand", Matrix::translation(4, 0, 0));
	Model tool;
	tool.skeleton().root().setBody(&volume("metal"));
	tool.skeleton().root().addJoint("grip", Matrix::translation(1, 0, 0));
	parent.skeleton().joint("hand").attach(tool, "grip");
	Model::View view(parent);
	spk::Texture atlas;
	auto &toolView = view.root().attachedModel("hand");
	toolView.bind(palette(atlas, {"metal"}));
	const auto *cached = toolView.root().geometry();
	auto *attached = parent.skeleton().joint("hand").model();
	ASSERT_NE(attached, nullptr);
	const auto parentBefore = parent.version();
	attached->skeleton().joint("grip").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	EXPECT_EQ(parent.version(), parentBefore);
	EXPECT_EQ(attached->skeleton().joint("grip").transform()[3][0], 1.0f);
	EXPECT_EQ(view.root().attachedModel("hand").root().geometry(), cached);
	parent.skeleton().joint("hand").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	EXPECT_GT(parent.version().pose, parentBefore.pose);
	EXPECT_EQ(view.root().attachedModel("hand").root().geometry(), cached);
}

TEST(ModelGraphicsTest, AttachingModelUpdatesViewAndPreservesExistingMeshes)
{
	Model character;
	character.skeleton().root().setBody(&volume("cloth"));
	character.skeleton().root().addJoint("hand", Matrix::translation(2, 0, 0));
	character.setTransform(Matrix::translation(10, 0, 0));
	Model sword;
	sword.skeleton().root().setBody(&volume("metal"));
	sword.skeleton().root().addJoint("grip", Matrix::translation(1, 0, 0));
	sword.setTransform(Matrix::translation(0, 3, 0));
	Model::View view(character);
	spk::Texture atlas;
	view.bind(palette(atlas, {"cloth"}));
	const auto *cached = view.root().geometry();
	character.skeleton().joint("hand").attach(sword, "grip");
	EXPECT_FALSE(view.isRenderable());
	auto &swordView = view.root().attachedModel("hand");
	EXPECT_EQ(swordView.model().skeleton().joint("grip").transform()[3][0], 1.0f);
	swordView.bind(palette(atlas, {"metal"}));
	EXPECT_TRUE(view.isRenderable());
	ASSERT_NE(swordView.root().geometry(), nullptr);
	EXPECT_EQ(swordView.root().geometry()->materialSlots()[0].name, "metal");
	EXPECT_EQ(view.root().geometry(), cached);
	character.skeleton().joint("hand").detach();
	EXPECT_THROW((void)view.root().attachedModel("hand"), spk::Exception);
	EXPECT_TRUE(view.isRenderable());
	EXPECT_EQ(view.root().geometry(), cached);
}

TEST(ModelGraphicsTest, ReplacingAttachedModelRebuildsOnlyItsBranch)
{
	Model character;
	character.skeleton().root().addJoint("hand");
	Model::View view(character);
	spk::Texture atlas;
	Model first;
	first.skeleton().root().setBody(&volume("steel"));
	character.skeleton().joint("hand").attach(first);
	view.root().attachedModel("hand").bind(palette(atlas, {"steel"}));
	EXPECT_TRUE(view.isRenderable());
	Model second;
	second.skeleton().root().setBody(&volume("wood"));
	character.skeleton().joint("hand").attach(second);
	EXPECT_FALSE(view.isRenderable());
	view.root().attachedModel("hand").bind(palette(atlas, {"wood"}));
	EXPECT_TRUE(view.isRenderable());
	character.skeleton().joint("hand").attach(Part(&volume("stone")));
	EXPECT_THROW((void)view.root().attachedModel("hand"), spk::Exception);
	view.bind(palette(atlas, {"stone"}));
	EXPECT_TRUE(view.isRenderable());
}

TEST(ModelGraphicsTest, NestedAttachedModelsKeepGraphicCachesAfterPoseChanges)
{
	Model parent;
	parent.skeleton().root().addJoint("hand");
	Model sword;
	sword.skeleton().root().addJoint("tip", Matrix::translation(3, 0, 0));
	Model gem;
	gem.skeleton().root().setBody(&volume("gem"));
	sword.skeleton().joint("tip").attach(gem);
	parent.skeleton().joint("hand").attach(sword);
	Model::View view(parent);
	spk::Texture atlas;
	auto &swordView = view.root().attachedModel("hand");
	auto &gemView = swordView.root().attachedModel("tip");
	gemView.bind(palette(atlas, {"gem"}));
	EXPECT_TRUE(view.isRenderable());
	const auto *gemGeometry = gemView.root().geometry();
	const auto parentBefore = parent.version();
	auto *attached = parent.skeleton().joint("hand").model();
	ASSERT_NE(attached, nullptr);
	const auto poseBefore = attached->version().pose;
	attached->skeleton().joint("tip").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	EXPECT_GT(attached->version().pose, poseBefore);
	EXPECT_EQ(parent.version(), parentBefore);
	EXPECT_EQ(view.root().attachedModel("hand").root().attachedModel("tip").root().geometry(), gemGeometry);
	EXPECT_TRUE(view.isRenderable());
}
