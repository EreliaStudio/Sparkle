#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "graphics/material.hpp"
#include "graphics/opengl/texture.hpp"
#include "voxel/palette.hpp"
#include "voxel/view.hpp"

namespace spk::Voxel
{
	struct ViewTestAccess
	{
		static void setNextID(View &view, PartID id)
		{
			view._nextID = id;
		}
	};
}

namespace
{
	using View = spk::Voxel::View;
	using PartID = spk::Voxel::PartID;
	using Matrix = spk::Matrix4x4;
	using Model = spk::Voxel::Model;

	static_assert(std::is_same_v<decltype(std::declval<View &>().part(PartID{})), const View::Part &>);
	static_assert(std::is_same_v<decltype(std::declval<const View::Part &>().models[0]), const Model &>);

	[[nodiscard]] Model model(std::string slot = "stone")
	{
		return Model({}, {}, {}, {{std::move(slot), 1.0f}});
	}

	[[nodiscard]] spk::Palette palette(const spk::Texture &atlas, std::vector<std::string> slots)
	{
		spk::Voxel::Palette::Builder builder;
		for (auto &slot : slots)
		{
			spk::Material::Builder material;
			material.add("albedo", atlas);
			builder.setMaterial(std::move(slot), material.build());
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

	[[nodiscard]] Matrix invalidMatrix()
	{
		auto value = Matrix::identity();
		value[3][0] = std::numeric_limits<float>::quiet_NaN();
		return value;
	}
}

TEST(VoxelViewTest, EmptyViewHasNoPaletteAndStartsAtVersionOne)
{
	View view;
	EXPECT_TRUE(view.empty());
	EXPECT_EQ(view.size(), 0u);
	EXPECT_EQ(view.version(), 1u);
	EXPECT_EQ(view.palette(), nullptr);
	EXPECT_TRUE(view.roots().empty());
	EXPECT_TRUE(view.partIDs().empty());
}

TEST(VoxelViewTest, SingleModelConstructorBuildsOneRoot)
{
	const auto geometry = model();
	View view(geometry);
	ASSERT_EQ(view.roots().size(), 1u);
	const auto root = view.roots()[0];
	ASSERT_EQ(view.part(root).models.size(), 1u);
	EXPECT_EQ(&view.part(root).models[0].opaqueMesh(), &geometry.opaqueMesh());
	EXPECT_EQ(view.palette(), nullptr);
	EXPECT_EQ(view.part(root).name, "model");
}

TEST(VoxelViewTest, SingleModelAndPaletteConstructorBindsGlobally)
{
	spk::Texture atlas;
	const auto geometry = model();
	const auto appearance = palette(atlas, {"stone"});
	View view(geometry, appearance);
	ASSERT_NE(view.palette(), nullptr);
	EXPECT_EQ(&view.palette()->materials(), &appearance.materials());
	EXPECT_EQ(view.size(), 1u);
	EXPECT_EQ(view.version(), 1u);
}

TEST(VoxelViewTest, SingleModelPaletteConstructorRejectsMissingSlots)
{
	spk::Palette empty;
	EXPECT_THROW((void)View(model(), empty), spk::Exception);
}

TEST(VoxelViewTest, MultipleModelsSharePartAndGeometry)
{
	auto geometry = model();
	View view;
	const auto body = view.addPart("body", {}, {geometry, geometry});
	ASSERT_EQ(view.part(body).models.size(), 2u);
	EXPECT_EQ(&view.part(body).models[0].opaqueMesh(), &view.part(body).models[1].opaqueMesh());
	view.addModel(body, geometry);
	ASSERT_EQ(view.part(body).models.size(), 3u);
	EXPECT_EQ(&view.part(body).models[2].opaqueMesh(), &geometry.opaqueMesh());
}

TEST(VoxelViewTest, MultiplePartsHaveIndependentTransformsButOnePalette)
{
	spk::Texture atlas;
	View view;
	const auto body = view.addPart("body", {}, {model("cloth"), model("armor")});
	const auto arm = view.addPart("arm", body, {model("skin"), model("armor")});
	const auto appearance = palette(atlas, {"cloth", "armor", "skin"});
	view.bind(appearance);
	ASSERT_NE(view.palette(), nullptr);
	EXPECT_EQ(&view.palette()->materials(), &appearance.materials());
	EXPECT_EQ(view.part(body).models.size(), 2u);
	EXPECT_EQ(view.part(arm).models.size(), 2u);
	view.setLocalTransform(arm, Matrix::translation(3, 0, 0));
	expectMatrix(view.accumulatedTransform(arm), Matrix::translation(3, 0, 0));
}

TEST(VoxelViewTest, AddsRootsChildrenAndEmptyAttachmentParts)
{
	View view;
	const auto torso = view.addPart("torso", {}, {model()});
	const auto left = view.addPart("arm", torso);
	const auto right = view.addPart("arm", torso, {model()});
	const auto hand = view.addPart("hand", left);
	const auto extra = view.addPart("other");
	EXPECT_EQ(view.roots(), (std::vector<PartID>{torso, extra}));
	EXPECT_EQ(view.children(torso), (std::vector<PartID>{left, right}));
	EXPECT_EQ(view.partIDs(), (std::vector<PartID>{torso, left, hand, right, extra}));
	EXPECT_TRUE(view.part(left).models.empty());
	EXPECT_EQ(view.part(right).models.size(), 1u);
}

TEST(VoxelViewTest, UnknownPartsAreRejected)
{
	View view;
	EXPECT_FALSE(view.contains(124));
	EXPECT_THROW((void)view.part(124), spk::Exception);
	EXPECT_THROW((void)view.children(124), spk::Exception);
	EXPECT_THROW((void)view.accumulatedTransform(124), spk::Exception);
	EXPECT_THROW(view.addModel(124, model()), spk::Exception);
	EXPECT_THROW(view.setModel(124, 0, model()), spk::Exception);
	EXPECT_THROW(view.removeModel(124, 0), spk::Exception);
	EXPECT_THROW(view.removePart(124), spk::Exception);
	EXPECT_THROW(view.setParent(124, std::nullopt), spk::Exception);
	EXPECT_THROW(view.setName(124, "new"), spk::Exception);
	EXPECT_THROW(view.setLocalTransform(124, Matrix::identity()), spk::Exception);
}

TEST(VoxelViewTest, ModelEditsThroughViewAreIndexedAndChecked)
{
	View view;
	const auto id = view.addPart("body", {}, {model("stone"), model("cloth")});
	const auto before = &view.part(id).models[0].opaqueMesh();
	view.setModel(id, 1, model("metal"));
	EXPECT_EQ(view.part(id).models[1].materialSlots()[0].name, "metal");
	EXPECT_EQ(&view.part(id).models[0].opaqueMesh(), before);
	EXPECT_THROW(view.setModel(id, 2, model()), spk::Exception);
	EXPECT_THROW(view.removeModel(id, 2), spk::Exception);
	EXPECT_EQ(view.part(id).models.size(), 2u);
	view.removeModel(id, 0);
	ASSERT_EQ(view.part(id).models.size(), 1u);
	EXPECT_EQ(view.part(id).models[0].materialSlots()[0].name, "metal");
}

TEST(VoxelViewTest, ModelMutationsRespectPaletteAfterBinding)
{
	spk::Texture atlas;
	View view;
	const auto id = view.addPart("body", {}, {model("skin")});
	view.bind(palette(atlas, {"skin", "cloth"}));
	view.addModel(id, model("cloth"));
	EXPECT_THROW(view.addModel(id, model("metal")), spk::Exception);
	EXPECT_THROW(view.setModel(id, 0, model("metal")), spk::Exception);
	EXPECT_EQ(view.part(id).models.size(), 2u);
	EXPECT_EQ(view.part(id).models[0].materialSlots()[0].name, "skin");
	EXPECT_EQ(view.part(id).models[1].materialSlots()[0].name, "cloth");
}

TEST(VoxelViewTest, AddingPartWithIncompatibleModelIsAtomic)
{
	spk::Texture atlas;
	View view;
	view.bind(palette(atlas, {"skin"}));
	const auto first = view.addPart("root");
	const auto version = view.version();
	EXPECT_THROW((void)view.addPart("invalid", first, {model("metal")}), spk::Exception);
	EXPECT_EQ(view.size(), 1u);
	EXPECT_EQ(view.version(), version);
	EXPECT_EQ(view.addPart("next"), first + 1);
}

TEST(VoxelViewTest, CanPopulateUnboundViewBeforeChoosingPalette)
{
	View view;
	const auto id = view.addPart("body", {}, {model("skin"), model("cloth")});
	view.addModel(id, model("armor"));
	EXPECT_EQ(view.palette(), nullptr);
	spk::Texture atlas;
	view.bind(palette(atlas, {"skin", "cloth", "armor"}));
	ASSERT_NE(view.palette(), nullptr);
	EXPECT_EQ(view.palette()->materials().size(), 3u);
}

TEST(VoxelViewTest, BindingIncompletePaletteLeavesPreviousBindingIntact)
{
	spk::Texture atlas;
	View view;
	const auto id = view.addPart("body", {}, {model("skin"), model("cloth")});
	const auto complete = palette(atlas, {"skin", "cloth"});
	view.bind(complete);
	const auto before = view.version();
	EXPECT_THROW(view.bind(palette(atlas, {"skin"})), spk::Exception);
	ASSERT_NE(view.palette(), nullptr);
	EXPECT_EQ(&view.palette()->materials(), &complete.materials());
	EXPECT_EQ(view.version(), before);
	EXPECT_EQ(view.part(id).models.size(), 2u);
}

TEST(VoxelViewTest, BindingReportsAllMissingSlots)
{
	spk::Texture atlas;
	View view;
	(void)view.addPart("body", {}, {model("alpha"), model("beta"), model("gamma")});
	try
	{
		view.bind(palette(atlas, {"beta"}));
		FAIL() << "Expected missing slots";
	} catch (const spk::Exception &error)
	{
		EXPECT_EQ(error.message(), "Voxel View: palette is missing material slots: 'alpha', 'gamma'");
	}
	EXPECT_EQ(view.palette(), nullptr);
}

TEST(VoxelViewTest, InvalidVoxelPaletteIsRejectedEvenWhenViewHasNoModels)
{
	spk::Texture first;
	spk::Texture second;
	spk::Material::Builder left;
	left.add("albedo", first);
	spk::Material::Builder right;
	right.add("albedo", second);
	spk::Palette::Builder generic;
	generic.setMaterial("a", left.build());
	generic.setMaterial("b", right.build());
	View view;
	EXPECT_THROW(view.bind(generic.build()), spk::Exception);
	EXPECT_EQ(view.palette(), nullptr);
}

TEST(VoxelViewTest, RebindingChangesAppearanceWithoutRebuildingGeometry)
{
	spk::Texture first;
	spk::Texture second;
	const auto geometry = model("stone");
	View view(geometry);
	const auto root = view.roots().front();
	const auto mesh = &view.part(root).models[0].opaqueMesh();
	const auto version = view.version();
	view.bind(palette(first, {"stone"}));
	view.bind(palette(second, {"stone"}));
	ASSERT_NE(view.palette(), nullptr);
	EXPECT_EQ(view.palette()->find("stone")->find("albedo")->texture.identifier(), second.handle().identifier());
	EXPECT_EQ(&view.part(root).models[0].opaqueMesh(), mesh);
	EXPECT_EQ(view.version(), version);
}

TEST(VoxelViewTest, ModelGeometryAndPaletteAreIndependentAcrossViews)
{
	spk::Texture first;
	spk::Texture second;
	const auto geometry = model();
	View left(geometry, palette(first, {"stone"}));
	View right(geometry, palette(second, {"stone"}));
	EXPECT_EQ(&left.part(left.roots()[0]).models[0].opaqueMesh(), &right.part(right.roots()[0]).models[0].opaqueMesh());
	EXPECT_NE(
		left.palette()->find("stone")->find("albedo")->texture.identifier(),
		right.palette()->find("stone")->find("albedo")->texture.identifier());
}

TEST(VoxelViewTest, ReparentPreservesLocalTransformAndRejectsCycles)
{
	View view;
	const auto root = view.addPart("root");
	const auto arm = view.addPart("arm", root, {}, Matrix::translation(1, 2, 3));
	const auto hand = view.addPart("hand", arm);
	const auto other = view.addPart("other");
	view.setParent(arm, other);
	EXPECT_EQ(view.part(arm).parent, other);
	expectMatrix(view.part(arm).localTransform, Matrix::translation(1, 2, 3));
	EXPECT_THROW(view.setParent(arm, arm), spk::Exception);
	EXPECT_THROW(view.setParent(other, hand), spk::Exception);
	EXPECT_THROW(view.setParent(root, 999), spk::Exception);
	EXPECT_EQ(view.part(other).parent, std::nullopt);
}

TEST(VoxelViewTest, TransformsAccumulateInParentFirstOrder)
{
	View view;
	const auto root = view.addPart("root", {}, {}, Matrix::translation(10, 0, 0));
	const auto arm = view.addPart("arm", root, {}, Matrix::rotation(0, 0, 90));
	const auto hand = view.addPart("hand", arm, {}, Matrix::scale(2, 3, 4));
	expectMatrix(
		view.accumulatedTransform(hand),
		Matrix::translation(10, 0, 0) * Matrix::rotation(0, 0, 90) * Matrix::scale(2, 3, 4));
}

TEST(VoxelViewTest, MutationsRequireExplicitInvalidation)
{
	spk::Texture atlas;
	View view;
	int calls = 0;
	auto contract = view.subscribeToVersionEdition([&](auto *) {
		++calls;
	});
	const auto before = view.version();
	const auto id = view.addPart("body", {}, {model()});
	view.setName(id, "torso");
	view.setLocalTransform(id, Matrix::translation(2, 0, 0));
	view.addModel(id, model());
	view.bind(palette(atlas, {"stone"}));
	EXPECT_EQ(view.version(), before);
	EXPECT_EQ(calls, 0);
	view.invalidate();
	EXPECT_EQ(view.version(), before + 1);
	EXPECT_EQ(calls, 1);
}

TEST(VoxelViewTest, RemovePartRemovesSubtreeButKeepsPalette)
{
	spk::Texture atlas;
	View view;
	view.bind(palette(atlas, {"stone"}));
	const auto root = view.addPart("root", {}, {model()});
	const auto child = view.addPart("child", root);
	const auto grandchild = view.addPart("grandchild", child);
	const auto independent = view.addPart("other");
	view.removePart(child);
	EXPECT_TRUE(view.contains(root));
	EXPECT_TRUE(view.contains(independent));
	EXPECT_FALSE(view.contains(child));
	EXPECT_FALSE(view.contains(grandchild));
	ASSERT_NE(view.palette(), nullptr);
	EXPECT_NE(view.palette()->find("stone"), nullptr);
	EXPECT_EQ(view.addPart("after"), independent + 1);
}

TEST(VoxelViewTest, EmptySourceInsertionCreatesOnlyAnchor)
{
	View source;
	View destination;
	const auto result = destination.insertView(std::nullopt, source);
	EXPECT_EQ(destination.size(), 1u);
	EXPECT_TRUE(destination.part(result.anchor).models.empty());
	EXPECT_TRUE(result.partIDs.empty());
	EXPECT_EQ(destination.roots(), (std::vector<PartID>{result.anchor}));
}

TEST(VoxelViewTest, InsertingViewCopiesAllModelsAndMultipleRoots)
{
	View source;
	const auto blade = source.addPart("blade", {}, {model("metal"), model("shine")});
	const auto grip = source.addPart("grip", {}, {model("wood")});
	const auto child = source.addPart("detail", blade, {model("metal")});
	View destination;
	const auto hand = destination.addPart("hand");
	const auto result = destination.insertView(hand, source, Matrix::translation(1, 0, 0), "sword");
	ASSERT_EQ(result.partIDs.size(), 3u);
	EXPECT_EQ(destination.part(result.anchor).name, "sword");
	EXPECT_EQ(destination.part(result.anchor).parent, hand);
	EXPECT_EQ(destination.part(result.partIDs.at(blade)).parent, result.anchor);
	EXPECT_EQ(destination.part(result.partIDs.at(grip)).parent, result.anchor);
	EXPECT_EQ(destination.part(result.partIDs.at(child)).parent, result.partIDs.at(blade));
	EXPECT_EQ(destination.part(result.partIDs.at(blade)).models.size(), 2u);
	expectMatrix(destination.accumulatedTransform(result.partIDs.at(blade)), Matrix::translation(1, 0, 0));
}

TEST(VoxelViewTest, InsertionIgnoresSourcePaletteButValidatesDestinationPalette)
{
	spk::Texture first;
	spk::Texture second;
	View source(model("blade"), palette(first, {"blade"}));
	View destination;
	destination.bind(palette(second, {"blade", "skin"}));
	const auto result = destination.insertView(std::nullopt, source);
	EXPECT_EQ(destination.part(result.partIDs.begin()->second).models.size(), 1u);
	ASSERT_NE(destination.palette(), nullptr);
	EXPECT_EQ(destination.palette()->find("blade")->find("albedo")->texture.identifier(), second.handle().identifier());
	EXPECT_EQ(source.palette()->find("blade")->find("albedo")->texture.identifier(), first.handle().identifier());
}

TEST(VoxelViewTest, IncompatibleInsertionPreservesDestinationAndNextID)
{
	spk::Texture atlas;
	View source(model("metal"));
	View destination;
	destination.bind(palette(atlas, {"skin"}));
	const auto first = destination.addPart("body");
	const auto original = destination.palette();
	const auto before = destination.version();
	EXPECT_THROW((void)destination.insertView(first, source), spk::Exception);
	EXPECT_EQ(destination.size(), 1u);
	EXPECT_EQ(destination.palette(), original);
	EXPECT_EQ(destination.version(), before);
	EXPECT_EQ(destination.addPart("next"), first + 1);
}

TEST(VoxelViewTest, RepeatedInsertionAssignsNewIDsAndHasIndependentSource)
{
	View source(model("blade"));
	View destination;
	const auto first = destination.insertView(std::nullopt, source);
	const auto second = destination.insertView(std::nullopt, source);
	EXPECT_NE(first.anchor, second.anchor);
	EXPECT_NE(first.partIDs.begin()->second, second.partIDs.begin()->second);
	source.removePart(source.roots()[0]);
	EXPECT_EQ(destination.part(first.partIDs.begin()->second).models.size(), 1u);
	EXPECT_EQ(destination.part(second.partIDs.begin()->second).models.size(), 1u);
}

TEST(VoxelViewTest, SelfInsertionAndInvalidTransformsPreserveState)
{
	View view;
	const auto root = view.addPart("root");
	const auto version = view.version();
	EXPECT_THROW((void)view.insertView(root, view), spk::Exception);
	EXPECT_THROW((void)view.addPart("invalid", {}, {}, invalidMatrix()), spk::Exception);
	EXPECT_THROW(view.setLocalTransform(root, invalidMatrix()), spk::Exception);
	View source(model());
	EXPECT_THROW((void)view.insertView(root, source, invalidMatrix()), spk::Exception);
	EXPECT_THROW((void)view.insertView(999, source), spk::Exception);
	EXPECT_EQ(view.size(), 1u);
	EXPECT_EQ(view.version(), version);
	EXPECT_EQ(view.addPart("after"), root + 1);
}

TEST(VoxelViewTest, CopyCreatesIndependentHierarchyPaletteAndVersion)
{
	spk::Texture first;
	spk::Texture second;
	View original(model(), palette(first, {"stone"}));
	const auto root = original.roots()[0];
	original.invalidate();
	View copy(original);
	EXPECT_EQ(copy.version(), 1u);
	EXPECT_EQ(&copy.palette()->materials(), &original.palette()->materials());
	copy.bind(palette(second, {"stone"}));
	copy.setName(root, "changed");
	EXPECT_EQ(original.part(root).name, "model");
	EXPECT_NE(
		copy.palette()->find("stone")->find("albedo")->texture.identifier(),
		original.palette()->find("stone")->find("albedo")->texture.identifier());
}

TEST(VoxelViewTest, CopyAssignmentResetsVersionAndSubscriptions)
{
	View original(model());
	original.invalidate();
	View destination;
	int notifications = 0;
	auto contract = destination.subscribeToVersionEdition([&](auto *) {
		++notifications;
	});
	destination = original;
	EXPECT_EQ(destination.version(), 1u);
	EXPECT_FALSE(contract.isValid());
	EXPECT_EQ(destination.size(), 1u);
	destination.invalidate();
	EXPECT_EQ(notifications, 0);
}

TEST(VoxelViewTest, SelfAssignmentKeepsSubscriptions)
{
	View view;
	const auto root = view.addPart("root");
	int calls = 0;
	auto contract = view.subscribeToVersionEdition([&](auto *) {
		++calls;
	});
	const auto version = view.version();
	view = view;
	EXPECT_TRUE(view.contains(root));
	EXPECT_EQ(view.version(), version);
	EXPECT_TRUE(contract.isValid());
	view.invalidate();
	EXPECT_EQ(calls, 1);
}

TEST(VoxelViewTest, DeletedPartIDsAreNotReused)
{
	View view;
	const auto first = view.addPart("first");
	const auto second = view.addPart("second");
	view.removePart(first);
	const auto third = view.addPart("third");
	EXPECT_LT(first, second);
	EXPECT_LT(second, third);
}

TEST(VoxelViewTest, ExhaustedPartIDsThrowWithoutMutatingView)
{
	View view;
	const auto first = view.addPart("first");
	const auto maximum = std::numeric_limits<PartID>::max();
	spk::Voxel::ViewTestAccess::setNextID(view, maximum);
	EXPECT_EQ(view.addPart("last"), maximum);
	EXPECT_THROW((void)view.addPart("overflow"), spk::Exception);
	EXPECT_EQ(view.size(), 2u);
	EXPECT_TRUE(view.contains(first));
}

TEST(VoxelViewTest, IDExhaustionDuringInsertionIsAtomic)
{
	View source(model());
	View destination;
	const auto first = destination.addPart("hand");
	const auto maximum = std::numeric_limits<PartID>::max();
	spk::Voxel::ViewTestAccess::setNextID(destination, maximum);
	const auto before = destination.version();
	EXPECT_THROW((void)destination.insertView(first, source), spk::Exception);
	EXPECT_EQ(destination.size(), 1u);
	EXPECT_EQ(destination.version(), before);
	EXPECT_EQ(destination.addPart("last"), maximum);
}

TEST(VoxelViewTest, MaterialIndicesMapModelsToSharedViewPalette)
{
	spk::Texture atlas;
	const Model torso({}, {}, {}, {{"cloth", 1.0f}, {"skin", 1.0f}});
	const Model sword({}, {}, {}, {{"metal", 1.0f}, {"skin", 1.0f}});
	View view;
	const auto body = view.addPart("body", {}, {torso, sword});
	view.bind(palette(atlas, {"skin", "metal", "cloth"}));
	EXPECT_EQ(view.materialIndices(body, 0), (std::vector<std::uint32_t>{0, 2}));
	EXPECT_EQ(view.materialIndices(body, 1), (std::vector<std::uint32_t>{1, 2}));
}

TEST(VoxelViewTest, MaterialIndicesAreResolvedFromSelectedPalette)
{
	spk::Texture atlas;
	View view;
	const auto id = view.addPart("body", {}, {model("skin"), model("metal")});
	EXPECT_THROW((void)view.materialIndices(id, 0), spk::Exception);
	view.bind(palette(atlas, {"skin", "metal"}));
	EXPECT_EQ(view.materialIndices(id, 0), (std::vector<std::uint32_t>{1}));
	EXPECT_EQ(view.materialIndices(id, 1), (std::vector<std::uint32_t>{0}));
	EXPECT_THROW((void)view.materialIndices(id, 2), spk::Exception);
	EXPECT_THROW((void)view.materialIndices(999, 0), spk::Exception);
}

TEST(VoxelViewTest, PaletteChangeRecomputesModelToPaletteIndices)
{
	spk::Texture atlas;
	View view;
	const auto id = view.addPart("body", {}, {model("skin")});
	view.bind(palette(atlas, {"skin"}));
	EXPECT_EQ(view.materialIndices(id, 0), (std::vector<std::uint32_t>{0}));
	view.bind(palette(atlas, {"cloth", "skin"}));
	EXPECT_EQ(view.materialIndices(id, 0), (std::vector<std::uint32_t>{1}));
	EXPECT_EQ(view.part(id).models.size(), 1u);
}
