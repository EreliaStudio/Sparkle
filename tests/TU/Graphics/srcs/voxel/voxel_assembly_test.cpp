#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "graphics/material.hpp"
#include "graphics/opengl/texture.hpp"
#include "graphics/palette.hpp"
#include "voxel/assembly.hpp"
#include "voxel/palette.hpp"

namespace spk::Voxel
{
	struct AssemblyTestAccess
	{
		static void setNextID(Assembly &assembly, PartID next)
		{
			assembly._nextID = next;
		}
	};
}

namespace
{
	using Assembly = spk::Voxel::Assembly;
	using PartID = spk::Voxel::PartID;
	using Matrix = spk::Matrix4x4;

	static_assert(std::is_same_v<decltype(std::declval<Assembly &>().part(PartID{})), const Assembly::Part &>);
	static_assert(std::is_same_v<decltype(*std::declval<const Assembly::Part &>().model), const spk::Voxel::Model &>);

	[[nodiscard]] spk::Palette palette(const spk::Texture &texture, std::string slot = "stone")
	{
		spk::Material::Builder material;
		material.add("albedo", texture);
		spk::Voxel::Palette::Builder builder;
		builder.setMaterial(std::move(slot), material.build());
		return builder.build();
	}

	[[nodiscard]] spk::Voxel::Model model(std::string slot = "stone")
	{
		static spk::Texture atlas;
		auto source = spk::Voxel::Model({}, {}, {}, {{slot, 1.0f}});
		source.bind(palette(atlas, std::move(slot)));
		return source;
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
		auto transform = Matrix::identity();
		transform[3][0] = std::numeric_limits<float>::quiet_NaN();
		return transform;
	}
}

TEST(VoxelAssemblyTest, EmptyAssemblyStartsAtVersionOne)
{
	Assembly assembly;
	EXPECT_TRUE(assembly.empty());
	EXPECT_EQ(assembly.size(), 0u);
	EXPECT_TRUE(assembly.roots().empty());
	EXPECT_TRUE(assembly.partIDs().empty());
	EXPECT_EQ(assembly.version(), 1u);
}

TEST(VoxelAssemblyTest, AddsRootsChildrenAndUnmodeledAttachments)
{
	Assembly assembly;
	const auto torso = assembly.addPart("torso", {}, model());
	const auto left = assembly.addPart("arm", torso);
	const auto right = assembly.addPart("arm", torso, model());
	const auto hand = assembly.addPart("hand", left);
	const auto independent = assembly.addPart("accessory");
	EXPECT_EQ(assembly.roots(), (std::vector<PartID>{torso, independent}));
	EXPECT_EQ(assembly.children(torso), (std::vector<PartID>{left, right}));
	EXPECT_EQ(assembly.partIDs(), (std::vector<PartID>{torso, left, hand, right, independent}));
	EXPECT_EQ(assembly.part(hand).parent, left);
	EXPECT_FALSE(assembly.part(left).model.has_value());
	EXPECT_TRUE(assembly.part(right).model.has_value());
	EXPECT_EQ(assembly.part(left).name, assembly.part(right).name);
}

TEST(VoxelAssemblyTest, UnknownPartsAreRejected)
{
	Assembly assembly;
	EXPECT_FALSE(assembly.contains(123));
	EXPECT_THROW((void)assembly.part(123), spk::Exception);
	EXPECT_THROW((void)assembly.children(123), spk::Exception);
	EXPECT_THROW((void)assembly.accumulatedTransform(123), spk::Exception);
	EXPECT_THROW(assembly.removePart(123), spk::Exception);
	EXPECT_THROW(assembly.setName(123, "new"), spk::Exception);
	EXPECT_THROW(assembly.setModel(123, std::nullopt), spk::Exception);
	EXPECT_THROW(assembly.setLocalTransform(123, Matrix::identity()), spk::Exception);
	EXPECT_THROW(assembly.setParent(123, std::nullopt), spk::Exception);
}

TEST(VoxelAssemblyTest, ReusesNeitherDeletedIDNorIdentity)
{
	Assembly assembly;
	const auto first = assembly.addPart("first");
	const auto second = assembly.addPart("second");
	assembly.removePart(first);
	const auto third = assembly.addPart("third");
	EXPECT_LT(first, second);
	EXPECT_LT(second, third);
	EXPECT_FALSE(assembly.contains(first));
	EXPECT_EQ(assembly.part(second).name, "second");
}

TEST(VoxelAssemblyTest, AccumulatesTranslationScaleAndRotationInParentOrder)
{
	Assembly assembly;
	const Matrix translation = Matrix::translation(10.0f, 0.0f, 0.0f);
	const Matrix rotation = Matrix::rotation(0.0f, 0.0f, 90.0f);
	const Matrix scale = Matrix::scale(2.0f, 3.0f, 4.0f);
	const auto torso = assembly.addPart("torso", {}, {}, translation);
	const auto arm = assembly.addPart("arm", torso, {}, rotation);
	const auto hand = assembly.addPart("hand", arm, {}, scale);
	const auto other = assembly.addPart("root", {}, {}, Matrix::translation(-5.0f, 0.0f, 0.0f));
	const auto expected = translation * rotation * scale;
	expectMatrix(assembly.accumulatedTransform(hand), expected);
	const auto position = assembly.accumulatedTransform(hand) * spk::Vector4{1, 0, 0, 1};
	EXPECT_NEAR(position.x, 10.0f, 0.0001f);
	EXPECT_NEAR(position.y, 2.0f, 0.0001f);
	EXPECT_NEAR(position.z, 0.0f, 0.0001f);
	expectMatrix(assembly.accumulatedTransform(other), Matrix::translation(-5.0f, 0.0f, 0.0f));
	EXPECT_NEAR((scale * translation * spk::Vector4{1, 0, 0, 1}).x, 22.0f, 0.0001f);
}

TEST(VoxelAssemblyTest, LocalTransformAndNameAreMutableWithoutAutomaticInvalidation)
{
	Assembly assembly;
	const auto root = assembly.addPart("root");
	const auto child = assembly.addPart("child", root);
	const auto version = assembly.version();
	assembly.setName(child, "hand");
	assembly.setLocalTransform(child, Matrix::translation(1, 2, 3));
	EXPECT_EQ(assembly.part(child).name, "hand");
	expectMatrix(assembly.part(child).localTransform, Matrix::translation(1, 2, 3));
	expectMatrix(assembly.accumulatedTransform(child), Matrix::translation(1, 2, 3));
	EXPECT_EQ(assembly.version(), version);
	assembly.invalidate();
	EXPECT_EQ(assembly.version(), version + 1);
}

TEST(VoxelAssemblyTest, ValidReparentingPreservesIDAndLocalPose)
{
	Assembly assembly;
	const auto first = assembly.addPart("first", {}, {}, Matrix::translation(2, 0, 0));
	const auto second = assembly.addPart("second", {}, {}, Matrix::scale(3, 1, 1));
	const auto arm = assembly.addPart("arm", first, {}, Matrix::translation(1, 0, 0));
	assembly.setParent(arm, second);
	EXPECT_EQ(assembly.part(arm).parent, second);
	EXPECT_EQ(assembly.part(arm).id, arm);
	expectMatrix(assembly.part(arm).localTransform, Matrix::translation(1, 0, 0));
	expectMatrix(assembly.accumulatedTransform(arm), Matrix::scale(3, 1, 1) * Matrix::translation(1, 0, 0));
	assembly.setParent(arm, std::nullopt);
	EXPECT_EQ(assembly.roots(), (std::vector<PartID>{first, second, arm}));
}

TEST(VoxelAssemblyTest, RejectsSelfDescendantAndUnknownParentWithoutMutation)
{
	Assembly assembly;
	const auto torso = assembly.addPart("torso");
	const auto arm = assembly.addPart("arm", torso);
	const auto hand = assembly.addPart("hand", arm);
	const auto before = assembly.partIDs();
	const auto version = assembly.version();
	EXPECT_THROW(assembly.setParent(torso, torso), spk::Exception);
	EXPECT_THROW(assembly.setParent(torso, hand), spk::Exception);
	EXPECT_THROW(assembly.setParent(arm, 67890), spk::Exception);
	EXPECT_EQ(assembly.partIDs(), before);
	EXPECT_FALSE(assembly.part(torso).parent.has_value());
	EXPECT_EQ(assembly.part(arm).parent, torso);
	EXPECT_EQ(assembly.version(), version);
}

TEST(VoxelAssemblyTest, RemovesEntireSubtreeWithoutChangingOtherParts)
{
	Assembly assembly;
	const auto torso = assembly.addPart("torso");
	const auto arm = assembly.addPart("arm", torso);
	const auto hand = assembly.addPart("hand", arm);
	const auto head = assembly.addPart("head", torso);
	const auto standalone = assembly.addPart("standalone");
	assembly.removePart(arm);
	EXPECT_FALSE(assembly.contains(arm));
	EXPECT_FALSE(assembly.contains(hand));
	EXPECT_EQ(assembly.partIDs(), (std::vector<PartID>{torso, head, standalone}));
	EXPECT_GT(assembly.addPart("next"), standalone);
}

TEST(VoxelAssemblyTest, CopyConstructionAndAssignmentHaveIndependentVersionAndNotifications)
{
	Assembly original;
	const auto torso = original.addPart("torso");
	original.invalidate();
	original.invalidate();
	int sourceEvents = 0;
	auto sourceContract = original.subscribeToVersionEdition(
		[&](auto *) {
			++sourceEvents;
		});
	Assembly copy(original);
	EXPECT_EQ(copy.version(), 1u);
	copy.setName(torso, "copy torso");
	copy.invalidate();
	EXPECT_EQ(original.part(torso).name, "torso");
	EXPECT_EQ(sourceEvents, 0);
	Assembly assigned;
	auto oldContract = assigned.subscribeToVersionEdition([](auto *) {
	});
	assigned = original;
	EXPECT_EQ(assigned.version(), 1u);
	EXPECT_FALSE(oldContract.isValid());
	assigned.invalidate();
	EXPECT_EQ(sourceEvents, 0);
	original.invalidate();
	EXPECT_EQ(sourceEvents, 1);
	EXPECT_TRUE(sourceContract.isValid());
}

TEST(VoxelAssemblyTest, CopiesRetainNextIDHistoryWithoutSharingMutableParts)
{
	Assembly original;
	const auto first = original.addPart("first");
	original.removePart(first);
	Assembly copy(original);
	const auto originalNext = original.addPart("original");
	const auto copyNext = copy.addPart("copy");
	EXPECT_EQ(originalNext, copyNext);
	copy.setLocalTransform(copyNext, Matrix::translation(2, 0, 0));
	EXPECT_EQ(original.part(originalNext).name, "original");
	EXPECT_EQ(copy.part(copyNext).name, "copy");
}

TEST(VoxelAssemblyTest, TwoModelsShareGeometryAndCommonAssemblyPalette)
{
	spk::Texture atlas;
	auto source = model();
	source.bind(palette(atlas));
	auto second = source;
	Assembly assembly;
	const auto torso = assembly.addPart("torso", {}, source);
	const auto arm = assembly.addPart("arm", torso, second);
	const auto &left = *assembly.part(torso).model;
	const auto &right = *assembly.part(arm).model;
	EXPECT_EQ(&left.opaqueMesh(), &right.opaqueMesh());
	EXPECT_EQ(&left.depthMesh(), &right.depthMesh());
	ASSERT_NE(left.palette(), nullptr);
	ASSERT_NE(right.palette(), nullptr);
	EXPECT_EQ(&left.palette()->materials(), &assembly.palette().materials());
	EXPECT_EQ(&right.palette()->materials(), &assembly.palette().materials());
}

TEST(VoxelAssemblyTest, MergesDistinctMaterialSlotsAndSharesResultWithEveryPart)
{
	spk::Texture atlas;
	auto torso = model("torso");
	torso.bind(palette(atlas, "torso"));
	auto arm = model("arm");
	arm.bind(palette(atlas, "arm"));
	Assembly assembly;
	const auto torsoID = assembly.addPart("torso", {}, torso);
	const auto armID = assembly.addPart("arm", torsoID, arm);
	EXPECT_EQ(assembly.palette().materials().size(), 2u);
	EXPECT_NE(assembly.palette().find("torso"), nullptr);
	EXPECT_NE(assembly.palette().find("arm"), nullptr);
	EXPECT_EQ(&assembly.part(torsoID).model->palette()->materials(), &assembly.palette().materials());
	EXPECT_EQ(&assembly.part(armID).model->palette()->materials(), &assembly.palette().materials());
	EXPECT_TRUE(assembly.part(torsoID).model->isCompatible(assembly.palette()));
	EXPECT_TRUE(assembly.part(armID).model->isCompatible(assembly.palette()));
}

TEST(VoxelAssemblyTest, RejectsDifferentAtlasesWithoutConsumingIDOrChangingPalette)
{
	spk::Texture first;
	spk::Texture second;
	auto torso = model();
	torso.bind(palette(first));
	auto arm = model("arm");
	arm.bind(palette(second, "arm"));
	Assembly assembly;
	const auto torsoID = assembly.addPart("torso", {}, torso);
	const auto originalVersion = assembly.version();
	const auto originalMaterials = &assembly.palette().materials();
	EXPECT_THROW((void)assembly.addPart("arm", torsoID, arm), spk::Exception);
	EXPECT_EQ(assembly.size(), 1u);
	EXPECT_EQ(assembly.version(), originalVersion);
	EXPECT_EQ(&assembly.palette().materials(), originalMaterials);
	EXPECT_EQ(assembly.addPart("attachment", torsoID), torsoID + 1);
}

TEST(VoxelAssemblyTest, RejectsConflictingMaterialDefinitionsForSameSlot)
{
	spk::Texture atlas;
	spk::Material::Builder secondMaterial;
	secondMaterial.add("albedo", atlas, {{0.5f, 0.0f}, {0.5f, 1.0f}});
	spk::Voxel::Palette::Builder secondPalette;
	secondPalette.setMaterial("stone", secondMaterial.build());
	auto original = model();
	original.bind(palette(atlas));
	auto variant = original;
	variant.bind(secondPalette.build());
	Assembly assembly;
	const auto first = assembly.addPart("first", {}, original);
	EXPECT_THROW((void)assembly.addPart("variant", first, variant), spk::Exception);
	EXPECT_EQ(assembly.size(), 1u);
	EXPECT_EQ(assembly.palette().materials().size(), 1u);
}

TEST(VoxelAssemblyTest, RejectsUnboundModelsAtomically)
{
	Assembly assembly;
	auto unbound = spk::Voxel::Model({}, {}, {}, {{"stone", 1.0f}});
	const auto version = assembly.version();
	EXPECT_THROW((void)assembly.addPart("unbound", {}, unbound), spk::Exception);
	EXPECT_EQ(assembly.version(), version);
	EXPECT_EQ(assembly.size(), 0u);
	EXPECT_EQ(assembly.addPart("attachment"), 1u);
}

TEST(VoxelAssemblyTest, ChangingModelsAndAnimatingPartsDoesNotModifyMeshData)
{
	Assembly assembly;
	auto source = model();
	const auto id = assembly.addPart("body", {}, source);
	const auto *mesh = &source.opaqueMesh();
	assembly.setLocalTransform(id, Matrix::translation(7, 8, 9));
	EXPECT_EQ(&assembly.part(id).model->opaqueMesh(), mesh);
	assembly.setModel(id, std::nullopt);
	EXPECT_FALSE(assembly.part(id).model.has_value());
	assembly.setModel(id, source);
	EXPECT_EQ(&assembly.part(id).model->opaqueMesh(), mesh);
}

TEST(VoxelAssemblyTest, InsertionPreservesParentsNamesAndSourceOrderWithSeveralRoots)
{
	Assembly sword;
	const auto blade = sword.addPart("blade", {}, model());
	const auto tip = sword.addPart("tip", blade);
	const auto guard = sword.addPart("guard");
	const auto grip = sword.addPart("grip", guard);
	Assembly character;
	const auto hand = character.addPart("hand");
	const auto result = character.insertAssembly(hand, sword);
	ASSERT_EQ(result.partIDs.size(), sword.size());
	EXPECT_EQ(character.part(result.anchor).parent, hand);
	EXPECT_FALSE(character.part(result.anchor).model.has_value());
	EXPECT_EQ(character.children(result.anchor), (std::vector<PartID>{result.partIDs.at(blade), result.partIDs.at(guard)}));
	EXPECT_EQ(character.part(result.partIDs.at(tip)).parent, result.partIDs.at(blade));
	EXPECT_EQ(character.part(result.partIDs.at(grip)).parent, result.partIDs.at(guard));
	EXPECT_EQ(character.part(result.partIDs.at(blade)).name, "blade");
	EXPECT_EQ(character.partIDs(), (std::vector<PartID>{hand, result.anchor, result.partIDs.at(blade), result.partIDs.at(tip), result.partIDs.at(guard), result.partIDs.at(grip)}));
}

TEST(VoxelAssemblyTest, InsertionAnchorComposesWithSourcePose)
{
	Assembly sword;
	const auto blade = sword.addPart("blade", {}, {}, Matrix::scale(2, 1, 1));
	Assembly character;
	const auto hand = character.addPart("hand", {}, {}, Matrix::translation(10, 0, 0));
	const auto anchorTransform = Matrix::rotation(0, 0, 90);
	const auto result = character.insertAssembly(hand, sword, anchorTransform, "sword mount");
	EXPECT_EQ(character.part(result.anchor).name, "sword mount");
	expectMatrix(character.accumulatedTransform(result.partIDs.at(blade)), Matrix::translation(10, 0, 0) * anchorTransform * Matrix::scale(2, 1, 1));
	expectMatrix(character.part(result.partIDs.at(blade)).localTransform, Matrix::scale(2, 1, 1));
}

TEST(VoxelAssemblyTest, EmptySourceCreatesOnlyAnchorEvenAtRoot)
{
	Assembly empty;
	Assembly destination;
	const auto result = destination.insertAssembly(std::nullopt, empty);
	EXPECT_EQ(destination.size(), 1u);
	EXPECT_EQ(destination.roots(), (std::vector<PartID>{result.anchor}));
	EXPECT_TRUE(result.partIDs.empty());
	EXPECT_FALSE(destination.part(result.anchor).model.has_value());
}

TEST(VoxelAssemblyTest, InsertingTwiceMakesIndependentIDsAndSubtrees)
{
	Assembly sword;
	const auto blade = sword.addPart("blade");
	Assembly destination;
	const auto first = destination.insertAssembly(std::nullopt, sword);
	const auto second = destination.insertAssembly(std::nullopt, sword);
	EXPECT_NE(first.anchor, second.anchor);
	EXPECT_NE(first.partIDs.at(blade), second.partIDs.at(blade));
	destination.setName(first.partIDs.at(blade), "renamed");
	EXPECT_EQ(destination.part(second.partIDs.at(blade)).name, "blade");
	destination.removePart(first.anchor);
	EXPECT_TRUE(destination.contains(second.partIDs.at(blade)));
	EXPECT_FALSE(destination.contains(first.partIDs.at(blade)));
}

TEST(VoxelAssemblyTest, SourceChangesAndDestructionNeverAffectImportedCopy)
{
	Assembly character;
	PartID copied = 0;
	const spk::Voxel::Mesh *mesh = nullptr;
	{
		Assembly source;
		const auto original = source.addPart("blade", {}, model());
		mesh = &source.part(original).model->opaqueMesh();
		const auto inserted = character.insertAssembly(std::nullopt, source);
		copied = inserted.partIDs.at(original);
		source.setName(original, "renamed source");
		source.removePart(original);
	}
	EXPECT_EQ(character.part(copied).name, "blade");
	EXPECT_EQ(&character.part(copied).model->opaqueMesh(), mesh);
}

TEST(VoxelAssemblyTest, SelfInsertionIsForbiddenAndPreservesState)
{
	Assembly assembly;
	const auto id = assembly.addPart("root");
	const auto version = assembly.version();
	EXPECT_THROW((void)assembly.insertAssembly(id, assembly), spk::Exception);
	EXPECT_EQ(assembly.size(), 1u);
	EXPECT_EQ(assembly.version(), version);
	EXPECT_EQ(assembly.addPart("after"), id + 1);
}

TEST(VoxelAssemblyTest, UnknownInsertionParentCannotConsumeID)
{
	Assembly source;
	(void)source.addPart("blade");
	Assembly destination;
	const auto first = destination.addPart("hand");
	const auto version = destination.version();
	EXPECT_THROW((void)destination.insertAssembly(9999, source), spk::Exception);
	EXPECT_EQ(destination.size(), 1u);
	EXPECT_EQ(destination.version(), version);
	EXPECT_EQ(destination.addPart("next"), first + 1);
}

TEST(VoxelAssemblyTest, InvalidTransformsNeverMutateOrConsumeIDs)
{
	Assembly source;
	const auto blade = source.addPart("blade");
	Assembly destination;
	const auto first = destination.addPart("first");
	const auto version = destination.version();
	EXPECT_THROW((void)destination.addPart("invalid", {}, {}, invalidMatrix()), spk::Exception);
	EXPECT_THROW(destination.setLocalTransform(first, invalidMatrix()), spk::Exception);
	EXPECT_THROW((void)destination.insertAssembly(std::nullopt, source, invalidMatrix()), spk::Exception);
	EXPECT_EQ(destination.version(), version);
	EXPECT_EQ(destination.size(), 1u);
	expectMatrix(destination.part(first).localTransform, Matrix::identity());
	EXPECT_EQ(destination.addPart("next"), first + 1);
	EXPECT_TRUE(source.contains(blade));
}

TEST(VoxelAssemblyTest, InsertionIsAnExplicitVisualInvalidationBoundary)
{
	Assembly source;
	(void)source.addPart("blade");
	Assembly destination;
	int notifications = 0;
	auto contract = destination.subscribeToVersionEdition([&](auto *) {
		++notifications;
	});
	const auto version = destination.version();
	const auto result = destination.insertAssembly(std::nullopt, source);
	destination.setLocalTransform(result.anchor, Matrix::translation(1, 2, 3));
	EXPECT_EQ(destination.version(), version);
	EXPECT_EQ(notifications, 0);
	destination.invalidate();
	EXPECT_EQ(notifications, 1);
	EXPECT_EQ(destination.version(), version + 1);
}

TEST(VoxelAssemblyTest, LastPartIDCanBeUsedExactlyOnce)
{
	Assembly assembly;
	const auto first = assembly.addPart("first");
	const auto maximum = std::numeric_limits<PartID>::max();
	spk::Voxel::AssemblyTestAccess::setNextID(assembly, maximum);
	EXPECT_EQ(assembly.addPart("last"), maximum);
	const auto version = assembly.version();
	EXPECT_THROW((void)assembly.addPart("overflow"), spk::Exception);
	EXPECT_EQ(assembly.size(), 2u);
	EXPECT_EQ(assembly.version(), version);
	EXPECT_TRUE(assembly.contains(first));
	EXPECT_TRUE(assembly.contains(maximum));
}

TEST(VoxelAssemblyTest, InsertionIDExhaustionIsAtomic)
{
	Assembly source;
	const auto blade = source.addPart("blade");
	Assembly destination;
	const auto first = destination.addPart("hand");
	const auto maximum = std::numeric_limits<PartID>::max();
	spk::Voxel::AssemblyTestAccess::setNextID(destination, maximum);
	const auto before = destination.version();
	EXPECT_THROW((void)destination.insertAssembly(first, source), spk::Exception);
	EXPECT_EQ(destination.size(), 1u);
	EXPECT_TRUE(destination.contains(first));
	EXPECT_EQ(destination.version(), before);
	EXPECT_EQ(destination.addPart("next"), maximum);
	EXPECT_FALSE(destination.contains(blade + 1));
}

TEST(VoxelAssemblyTest, SelfAssignmentKeepsExistingSubscriptions)
{
	Assembly assembly;
	const auto id = assembly.addPart("root");
	const auto before = assembly.version();
	int calls = 0;
	auto contract = assembly.subscribeToVersionEdition([&](auto *) {
		++calls;
	});
	assembly = assembly;
	EXPECT_EQ(assembly.version(), before);
	EXPECT_TRUE(contract.isValid());
	EXPECT_TRUE(assembly.contains(id));
	assembly.invalidate();
	EXPECT_EQ(calls, 1);
}

TEST(VoxelAssemblyTest, RemovingPartRebuildsPaletteWithoutOrphanedMaterials)
{
	spk::Texture atlas;
	auto torso = model("torso");
	torso.bind(palette(atlas, "torso"));
	auto arm = model("arm");
	arm.bind(palette(atlas, "arm"));
	Assembly assembly;
	const auto first = assembly.addPart("torso", {}, torso);
	const auto second = assembly.addPart("arm", first, arm);
	ASSERT_EQ(assembly.palette().materials().size(), 2u);
	assembly.removePart(second);
	ASSERT_NE(assembly.part(first).model->palette(), nullptr);
	EXPECT_EQ(assembly.palette().materials().size(), 1u);
	EXPECT_NE(assembly.palette().find("torso"), nullptr);
	EXPECT_EQ(assembly.palette().find("arm"), nullptr);
	EXPECT_EQ(&assembly.part(first).model->palette()->materials(), &assembly.palette().materials());
	assembly.removePart(first);
	EXPECT_TRUE(assembly.palette().materials().empty());
}

TEST(VoxelAssemblyTest, ReplacingModelRebuildsCommonPalette)
{
	spk::Texture atlas;
	auto torso = model("torso");
	torso.bind(palette(atlas, "torso"));
	auto arm = model("arm");
	arm.bind(palette(atlas, "arm"));
	Assembly assembly;
	const auto id = assembly.addPart("torso", {}, torso);
	assembly.setModel(id, arm);
	EXPECT_EQ(assembly.palette().find("torso"), nullptr);
	EXPECT_NE(assembly.palette().find("arm"), nullptr);
	ASSERT_NE(assembly.part(id).model->palette(), nullptr);
	EXPECT_EQ(&assembly.part(id).model->palette()->materials(), &assembly.palette().materials());
	assembly.setModel(id, std::nullopt);
	EXPECT_TRUE(assembly.palette().materials().empty());
}

TEST(VoxelAssemblyTest, ConflictingModelReplacementPreservesOriginal)
{
	spk::Texture first;
	spk::Texture second;
	auto torso = model("torso");
	torso.bind(palette(first, "torso"));
	auto arm = model("arm");
	arm.bind(palette(first, "arm"));
	auto replacement = model("replacement");
	replacement.bind(palette(second, "replacement"));
	Assembly assembly;
	const auto torsoID = assembly.addPart("torso", {}, torso);
	const auto armID = assembly.addPart("arm", torsoID, arm);
	const auto originalVersion = assembly.version();
	EXPECT_THROW(assembly.setModel(armID, replacement), spk::Exception);
	EXPECT_NE(assembly.palette().find("arm"), nullptr);
	EXPECT_EQ(assembly.palette().find("replacement"), nullptr);
	EXPECT_EQ(assembly.part(armID).model->materialSlots()[0].name, "arm");
	EXPECT_EQ(assembly.version(), originalVersion);
}

TEST(VoxelAssemblyTest, InsertionMergesPalettesAndPreservesSourceIndependence)
{
	spk::Texture atlas;
	auto blade = model("blade");
	blade.bind(palette(atlas, "blade"));
	auto handle = model("handle");
	handle.bind(palette(atlas, "handle"));
	Assembly sword;
	const auto bladeID = sword.addPart("blade", {}, blade);
	const auto handleID = sword.addPart("handle", bladeID, handle);
	Assembly character;
	const auto inserted = character.insertAssembly(std::nullopt, sword);
	EXPECT_EQ(character.palette().materials().size(), 2u);
	EXPECT_EQ(&character.part(inserted.partIDs.at(bladeID)).model->palette()->materials(), &character.palette().materials());
	EXPECT_EQ(&character.part(inserted.partIDs.at(handleID)).model->palette()->materials(), &character.palette().materials());
	sword.removePart(handleID);
	EXPECT_EQ(sword.palette().materials().size(), 1u);
	EXPECT_EQ(character.palette().materials().size(), 2u);
}

TEST(VoxelAssemblyTest, ConflictingInsertionKeepsDestinationUnchanged)
{
	spk::Texture first;
	spk::Texture second;
	auto sourceModel = model("blade");
	sourceModel.bind(palette(second, "blade"));
	Assembly source;
	(void)source.addPart("blade", {}, sourceModel);
	auto body = model("body");
	body.bind(palette(first, "body"));
	Assembly character;
	const auto existing = character.addPart("body", {}, body);
	const auto version = character.version();
	const auto initialMaterials = &character.palette().materials();
	EXPECT_THROW((void)character.insertAssembly(existing, source), spk::Exception);
	EXPECT_EQ(character.version(), version);
	EXPECT_EQ(character.size(), 1u);
	EXPECT_EQ(&character.palette().materials(), initialMaterials);
	EXPECT_EQ(character.addPart("next"), existing + 1);
}

TEST(VoxelAssemblyTest, CopiedAssemblyKeepsIndependentMergedPalette)
{
	spk::Texture atlas;
	auto torso = model("torso");
	torso.bind(palette(atlas, "torso"));
	Assembly original;
	const auto id = original.addPart("torso", {}, torso);
	Assembly copy(original);
	EXPECT_EQ(copy.version(), 1u);
	EXPECT_EQ(&copy.palette().materials(), &original.palette().materials());
	auto arm = model("arm");
	arm.bind(palette(atlas, "arm"));
	const auto armID = copy.addPart("arm", id, arm);
	EXPECT_NE(copy.palette().find("arm"), nullptr);
	EXPECT_EQ(original.palette().find("arm"), nullptr);
	EXPECT_EQ(&copy.part(armID).model->palette()->materials(), &copy.palette().materials());
	EXPECT_EQ(&original.part(id).model->palette()->materials(), &original.palette().materials());
}

TEST(VoxelAssemblyTest, BindingAssemblyPaletteChangesEveryModelButNotGeometry)
{
	spk::Texture initialAtlas;
	spk::Texture replacementAtlas;
	auto torso = model("torso");
	auto arm = model("arm");
	torso.bind(palette(initialAtlas, "torso"));
	arm.bind(palette(initialAtlas, "arm"));
	Assembly assembly;
	const auto torsoID = assembly.addPart("torso", {}, torso);
	const auto armID = assembly.addPart("arm", torsoID, arm);
	const auto *torsoGeometry = &torso.opaqueMesh();
	const auto *armGeometry = &arm.opaqueMesh();
	const auto version = assembly.version();

	spk::Material::Builder material;
	material.add("albedo", replacementAtlas);
	spk::Voxel::Palette::Builder builder;
	builder.setMaterial("torso", material.build());
	builder.setMaterial("arm", material.build());
	const auto replacement = builder.build();
	assembly.bind(replacement);

	EXPECT_EQ(&assembly.palette().materials(), &replacement.materials());
	ASSERT_NE(assembly.part(torsoID).model->palette(), nullptr);
	ASSERT_NE(assembly.part(armID).model->palette(), nullptr);
	EXPECT_EQ(&assembly.part(torsoID).model->palette()->materials(), &replacement.materials());
	EXPECT_EQ(&assembly.part(armID).model->palette()->materials(), &replacement.materials());
	EXPECT_EQ(&assembly.part(torsoID).model->opaqueMesh(), torsoGeometry);
	EXPECT_EQ(&assembly.part(armID).model->opaqueMesh(), armGeometry);
	EXPECT_EQ(assembly.version(), version);
	EXPECT_EQ(torso.palette()->find("torso")->find("albedo")->texture.identifier(), initialAtlas.handle().identifier());
	EXPECT_EQ(arm.palette()->find("arm")->find("albedo")->texture.identifier(), initialAtlas.handle().identifier());
}

TEST(VoxelAssemblyTest, BindingIncompletePaletteDoesNotChangeAnyPart)
{
	spk::Texture atlas;
	Assembly assembly;
	const auto torsoID = assembly.addPart("torso", {}, model("torso"));
	const auto armID = assembly.addPart("arm", torsoID, model("arm"));
	const auto version = assembly.version();
	const auto *previousMaterials = &assembly.palette().materials();
	const auto previousIDs = assembly.partIDs();
	const auto missingArm = palette(atlas, "torso");
	EXPECT_THROW(assembly.bind(missingArm), spk::Exception);
	EXPECT_EQ(assembly.version(), version);
	EXPECT_EQ(assembly.partIDs(), previousIDs);
	EXPECT_EQ(&assembly.palette().materials(), previousMaterials);
	EXPECT_EQ(&assembly.part(torsoID).model->palette()->materials(), previousMaterials);
	EXPECT_EQ(&assembly.part(armID).model->palette()->materials(), previousMaterials);
}

TEST(VoxelAssemblyTest, BindingRejectsGenericPaletteWithSeveralAlbedoAtlases)
{
	spk::Texture first;
	spk::Texture second;
	spk::Material::Builder firstMaterial;
	firstMaterial.add("albedo", first);
	spk::Material::Builder secondMaterial;
	secondMaterial.add("albedo", second);
	spk::Palette::Builder generic;
	generic.setMaterial("torso", firstMaterial.build());
	generic.setMaterial("arm", secondMaterial.build());
	Assembly assembly;
	const auto version = assembly.version();
	EXPECT_THROW(assembly.bind(generic.build()), spk::Exception);
	EXPECT_EQ(assembly.version(), version);
	EXPECT_TRUE(assembly.palette().materials().empty());
}

TEST(VoxelAssemblyTest, ExplicitPaletteAllowsInitiallyUnboundModels)
{
	spk::Texture atlas;
	Assembly assembly;
	auto unbound = spk::Voxel::Model({}, {}, {}, {{"stone", 1.0f}});
	ASSERT_EQ(unbound.palette(), nullptr);
	assembly.bind(palette(atlas));
	const auto id = assembly.addPart("stone", {}, unbound);
	ASSERT_NE(assembly.part(id).model->palette(), nullptr);
	EXPECT_EQ(&assembly.part(id).model->palette()->materials(), &assembly.palette().materials());
	EXPECT_EQ(unbound.palette(), nullptr);
}

TEST(VoxelAssemblyTest, ExplicitPalettePreservedDuringStructuralMutations)
{
	spk::Texture sourceAtlas;
	spk::Texture chosenAtlas;
	auto torsoModel = model("stone");
	torsoModel.bind(palette(sourceAtlas));
	Assembly assembly;
	const auto torso = assembly.addPart("torso", {}, torsoModel);
	const auto chosen = palette(chosenAtlas);
	assembly.bind(chosen);

	auto alternativeModel = model("stone");
	alternativeModel.bind(palette(sourceAtlas));
	const auto arm = assembly.addPart("arm", torso, alternativeModel);
	EXPECT_EQ(&assembly.part(arm).model->palette()->materials(), &chosen.materials());
	assembly.setModel(arm, spk::Voxel::Model({}, {}, {}, {{"stone", 1.0f}}));
	EXPECT_EQ(&assembly.part(arm).model->palette()->materials(), &chosen.materials());

	Assembly sword;
	(void)sword.addPart("blade", {}, alternativeModel);
	const auto inserted = assembly.insertAssembly(arm, sword);
	EXPECT_EQ(&assembly.part(inserted.partIDs.begin()->second).model->palette()->materials(), &chosen.materials());
	assembly.removePart(arm);
	EXPECT_EQ(&assembly.palette().materials(), &chosen.materials());
	EXPECT_EQ(&assembly.part(torso).model->palette()->materials(), &chosen.materials());
}

TEST(VoxelAssemblyTest, ExplicitPaletteRejectsMissingSlotAfterModelReplacement)
{
	spk::Texture atlas;
	Assembly assembly;
	const auto id = assembly.addPart("root");
	const auto chosen = palette(atlas);
	assembly.bind(chosen);
	const auto version = assembly.version();
	const auto different = model("other");
	EXPECT_THROW(assembly.setModel(id, different), spk::Exception);
	EXPECT_EQ(assembly.version(), version);
	EXPECT_FALSE(assembly.part(id).model.has_value());
	EXPECT_EQ(&assembly.palette().materials(), &chosen.materials());
}

TEST(VoxelAssemblyTest, ExplicitPaletteRemainsIndependentWhenCopied)
{
	spk::Texture initial;
	spk::Texture updated;
	auto initialModel = model();
	initialModel.bind(palette(initial));
	Assembly original;
	const auto id = original.addPart("root", {}, initialModel);
	original.bind(palette(updated));
	Assembly copy(original);
	const auto version = original.version();
	copy.bind(palette(initial));
	EXPECT_EQ(copy.version(), 1u);
	EXPECT_EQ(original.version(), version);
	EXPECT_EQ(&original.part(id).model->palette()->materials(), &original.palette().materials());
	EXPECT_EQ(&copy.part(id).model->palette()->materials(), &copy.palette().materials());
	EXPECT_NE(original.palette().find("stone")->find("albedo")->texture.identifier(), copy.palette().find("stone")->find("albedo")->texture.identifier());
}

TEST(VoxelAssemblyTest, InsertionAfterExplicitBindingUsesSourceAssemblyPalette)
{
	spk::Texture initial;
	spk::Texture selected;
	auto originalModel = model();
	originalModel.bind(palette(initial));
	Assembly source;
	const auto sourceID = source.addPart("blade", {}, originalModel);
	source.bind(palette(selected));

	Assembly destination;
	const auto result = destination.insertAssembly(std::nullopt, source);
	const auto insertedID = result.partIDs.at(sourceID);
	EXPECT_EQ(&destination.part(insertedID).model->palette()->materials(), &destination.palette().materials());
	EXPECT_EQ(destination.palette().find("stone")->find("albedo")->texture.identifier(), selected.handle().identifier());
}
