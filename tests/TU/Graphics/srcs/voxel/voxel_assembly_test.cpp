#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "graphics/material.hpp"
#include "graphics/opengl/texture.hpp"
#include "graphics/palette.hpp"
#include "voxel/assembly.hpp"
#include "voxel/palette.hpp"

namespace
{
	using Assembly = spk::Voxel::Assembly;
	using PartID = spk::Voxel::PartID;
	using Matrix = spk::Matrix4x4;

	[[nodiscard]] spk::Voxel::Model model(std::string slot = "stone")
	{
		return spk::Voxel::Model({}, {}, {}, {{std::move(slot), 1.0f}});
	}

	[[nodiscard]] spk::Palette palette(const spk::Texture &texture)
	{
		spk::Material::Builder material;
		material.add("albedo", texture);
		spk::Voxel::Palette::Builder builder;
		builder.setMaterial("stone", material.build());
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
	auto sourceContract = original.subscribeToVersionEdition([&](auto *) {
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

TEST(VoxelAssemblyTest, TwoModelCopiesShareGeometryButMayBindDifferentPalettes)
{
	spk::Texture firstAtlas;
	spk::Texture secondAtlas;
	auto source = model();
	source.bind(palette(firstAtlas));
	auto second = source;
	second.bind(palette(secondAtlas));
	Assembly assembly;
	const auto torso = assembly.addPart("torso", {}, source);
	const auto arm = assembly.addPart("arm", torso, second);
	const auto &left = *assembly.part(torso).model;
	const auto &right = *assembly.part(arm).model;
	EXPECT_EQ(&left.opaqueMesh(), &right.opaqueMesh());
	EXPECT_EQ(&left.depthMesh(), &right.depthMesh());
	ASSERT_NE(left.palette(), nullptr);
	ASSERT_NE(right.palette(), nullptr);
	EXPECT_NE(&left.palette()->materials(), &right.palette()->materials());
	EXPECT_NE(left.palette()->find("stone")->find("albedo")->texture.identifier(), right.palette()->find("stone")->find("albedo")->texture.identifier());
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
