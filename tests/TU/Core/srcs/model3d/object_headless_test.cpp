#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include "exception.hpp"
#include "model3d/object_headless.hpp"

namespace
{
	spk::Model3D::Blueprint armAssembly()
	{
		spk::Model3D::Blueprint::Builder builder;
		auto &body = builder.setRoot<spk::Model3D::Blueprint::Element>("body");
		body.setAnchor({1, 0, 0});
		body.addAttachmentPoint("shoulder", {2, 0, 0});
		auto &arm = body.attach<spk::Model3D::Blueprint::Element>("shoulder", "arm");
		arm.setAnchor({0, 1, 0});
		arm.addAttachmentPoint("wrist", {1, 0, 0});
		arm.attach<spk::Model3D::Blueprint::Element>("wrist", "hand").setAnchor({1, 0, 0});
		body.addAttachmentPoint("grip", {3, 0, 0});
		return builder.build();
	}

	spk::Model3D::Blueprint swordAssembly()
	{
		spk::Model3D::Blueprint::Builder builder;
		auto &blade = builder.setRoot<spk::Model3D::Blueprint::Element>("blade");
		blade.setAnchor({0, 2, 0});
		blade.addAttachmentPoint("guard", {0, 1, 0});
		return builder.build();
	}
}

using ReadOnlyChildren = const std::vector<const spk::Model3D::Attachable *> &;
using ActualChildren = decltype(std::declval<const spk::Model3D::Attachable &>().children());
static_assert(std::is_same_v<ActualChildren, ReadOnlyChildren>);

TEST(ObjectHeadlessTest, UsesSharedDefinitionsAndIndependentGlobalTransforms)
{
	const auto definition = armAssembly();
	spk::Model3D::ObjectHeadless first(definition);
	spk::Model3D::ObjectHeadless second(definition);

	EXPECT_EQ(&first.root().definition().anchor(), &second.root().definition().anchor());
	EXPECT_EQ(first.parentAttachment(), nullptr);
	EXPECT_EQ(first.children().size(), 1u);
	EXPECT_EQ(first.children().front(), &first.root());
	EXPECT_EQ(first.root().children().size(), 1u);
	EXPECT_EQ(first.root().children().front(), &first.element("arm"));
	EXPECT_EQ(first.element("arm").parentAttachment(), &first.attachment("body", "shoulder"));
	EXPECT_FLOAT_EQ(first.element("arm").globalTransform()[3][0], 3.0f);
	EXPECT_FLOAT_EQ(first.element("arm").globalTransform()[3][1], 1.0f);
	EXPECT_FLOAT_EQ(first.element("hand").globalTransform()[3][0], 5.0f);

	first.attachment("body", "shoulder").setRotation(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(first.element("arm").globalTransform()[3][0], 2.0f, 0.0001f);
	EXPECT_NEAR(first.element("arm").globalTransform()[3][1], 0.0f, 0.0001f);
	EXPECT_NEAR(first.element("hand").globalTransform()[3][0], 2.0f, 0.0001f);
	EXPECT_NEAR(first.element("hand").globalTransform()[3][1], 2.0f, 0.0001f);
	EXPECT_FLOAT_EQ(second.element("arm").globalTransform()[3][0], 3.0f);
}

TEST(ObjectHeadlessTest, RotatesLocallyAndKeepsJointPivotFixed)
{
	spk::Model3D::ObjectHeadless pose(armAssembly());
	auto &shoulder = pose.attachment("body", "shoulder");
	const auto quarterTurn = spk::Quaternion::fromAxisAngle({0, 0, 1}, 90.0f);
	shoulder.setRotation(quarterTurn);
	shoulder.rotate(quarterTurn);

	EXPECT_NEAR(shoulder.rotationMatrix()[0][0], -1.0f, 0.0001f);
	EXPECT_NEAR(shoulder.transform()[3][0], 2.0f, 0.0001f);
	EXPECT_NEAR(shoulder.transform()[3][1], 0.0f, 0.0001f);
	EXPECT_NEAR(pose.element("arm").globalTransform()[3][0], 3.0f, 0.0001f);
	EXPECT_NEAR(pose.element("arm").globalTransform()[3][1], -1.0f, 0.0001f);

	shoulder.setRotation(spk::Vector3{});
	EXPECT_NEAR(shoulder.rotationMatrix()[0][0], 1.0f, 0.0001f);
	EXPECT_NEAR(pose.element("arm").globalTransform()[3][1], 1.0f, 0.0001f);
}

TEST(ObjectHeadlessTest, RejectsInvalidRotationsWithoutChangingPose)
{
	spk::Model3D::ObjectHeadless pose(armAssembly());
	auto &shoulder = pose.attachment("body", "shoulder");
	shoulder.setRotation(spk::Vector3{0, 0, 90});
	const float before = pose.element("arm").globalTransform()[3][0];

	EXPECT_THROW(shoulder.setRotation(spk::Quaternion{0, 0, 0, 0}), spk::Exception);
	EXPECT_THROW(shoulder.rotate(spk::Vector3{std::numeric_limits<float>::infinity(), 0, 0}), spk::Exception);
	EXPECT_NEAR(pose.element("arm").globalTransform()[3][0], before, 0.0001f);
}

TEST(ObjectHeadlessTest, TraversesOnlyDirectChildrenInStructuralThenInsertionOrder)
{
	spk::Model3D::ObjectHeadless character(armAssembly());
	auto &shoulder = character.attachment("body", "shoulder");
	auto &grip = character.attachment("body", "grip");
	auto &sword = grip.attach(swordAssembly());
	auto &shield = shoulder.attach(swordAssembly());
	auto &dagger = grip.attach(swordAssembly());

	ASSERT_EQ(character.children().size(), 1u);
	EXPECT_EQ(character.children()[0], &character.root());
	ASSERT_EQ(character.root().children().size(), 4u);
	EXPECT_EQ(character.root().children()[0], &character.element("arm"));
	EXPECT_EQ(character.root().children()[1], &sword);
	EXPECT_EQ(character.root().children()[2], &shield);
	EXPECT_EQ(character.root().children()[3], &dagger);
	EXPECT_EQ(shoulder.attachedCount(), 1u);
	EXPECT_EQ(grip.attachedCount(), 2u);
	EXPECT_EQ(sword.children()[0], &sword.root());
	EXPECT_EQ(character.element("arm").children()[0], &character.element("hand"));
}

TEST(ObjectHeadlessTest, AttachesMultipleAccessoriesToStructuralAttachment)
{
	spk::Model3D::ObjectHeadless character(armAssembly());
	auto &shoulder = character.attachment("body", "shoulder");
	auto &sword = shoulder.attach(swordAssembly());
	auto &shield = shoulder.attach(swordAssembly());
	const auto *swordRoot = &sword.root();

	EXPECT_EQ(shoulder.attachedCount(), 2u);
	EXPECT_EQ(sword.parentAttachment(), &shoulder);
	EXPECT_EQ(shield.parentAttachment(), &shoulder);
	EXPECT_EQ(character.root().children()[0], &character.element("arm"));
	shoulder.setRotation(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(sword.root().globalTransform()[3][0], 1.0f, 0.0001f);
	EXPECT_NEAR(sword.root().globalTransform()[3][1], 0.0f, 0.0001f);
	EXPECT_NEAR(shield.root().globalTransform()[3][0], 1.0f, 0.0001f);
	EXPECT_EQ(&sword.root(), swordRoot);
}

TEST(ObjectHeadlessTest, InvalidatesOnlyArticulatedSubtreeAndPreservesOtherBranch)
{
	spk::Model3D::ObjectHeadless character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &sword = grip.attach(swordAssembly());
	const auto beforeRoot = character.root().globalTransform();
	const auto beforeSword = sword.root().globalTransform();

	character.attachment("body", "shoulder").setRotation(spk::Vector3{0, 0, 90});
	EXPECT_EQ(character.root().globalTransform()[3][0], beforeRoot[3][0]);
	EXPECT_EQ(sword.root().globalTransform()[3][0], beforeSword[3][0]);
	EXPECT_NEAR(character.element("hand").globalTransform()[3][1], 2.0f, 0.0001f);

	grip.setRotation(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(sword.root().globalTransform()[3][0], 2.0f, 0.0001f);
	EXPECT_NEAR(sword.root().globalTransform()[3][1], 0.0f, 0.0001f);
}

TEST(ObjectHeadlessTest, ReusesCachedTransformUntilPoseChanges)
{
	spk::Model3D::ObjectHeadless character(armAssembly());
	const auto &first = character.element("arm").globalTransform();
	const auto &second = character.element("arm").globalTransform();
	EXPECT_EQ(&first, &second);
	EXPECT_NEAR(first[3][0], 3.0f, 0.0001f);

	character.attachment("body", "shoulder").setRotation(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(character.element("arm").globalTransform()[3][0], 2.0f, 0.0001f);
}

TEST(ObjectHeadlessTest, DetachesSpecificAccessoryWithoutAffectingOthers)
{
	spk::Model3D::ObjectHeadless character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &sword = grip.attach(swordAssembly());
	auto &shield = grip.attach(swordAssembly());
	const auto *originalRoot = &sword.root();
	const auto *otherRoot = &shield.root();
	grip.rotate(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(sword.root().globalTransform()[3][0], 2.0f, 0.0001f);

	spk::Model3D::ObjectHeadless detached = grip.detach(sword);
	EXPECT_EQ(detached.parentAttachment(), nullptr);
	EXPECT_EQ(&detached.root(), originalRoot);
	EXPECT_EQ(grip.attachedCount(), 1u);
	EXPECT_EQ(&shield.root(), otherRoot);
	EXPECT_EQ(shield.parentAttachment(), &grip);
	EXPECT_NEAR(detached.root().globalTransform()[3][0], 0.0f, 0.0001f);
	EXPECT_NEAR(detached.root().globalTransform()[3][1], 2.0f, 0.0001f);
	EXPECT_EQ(character.root().children().size(), 2u);
}

TEST(ObjectHeadlessTest, DetachThenReattachKeepsRecursiveArticulations)
{
	spk::Model3D::ObjectHeadless character(armAssembly());
	spk::Model3D::ObjectHeadless sword(swordAssembly());
	sword.attachment("blade", "guard").setRotation(spk::Vector3{0, 0, 90});
	auto &gem = sword.attachment("blade", "guard").attach(swordAssembly());
	gem.attachment("blade", "guard").rotate(spk::Vector3{0, 0, 45});

	auto &grip = character.attachment("body", "grip");
	auto &attached = grip.attach(std::move(sword));
	const auto *innerRoot = &attached.root();
	spk::Model3D::ObjectHeadless detached = grip.detach(attached);
	EXPECT_EQ(detached.attachment("blade", "guard").attachedCount(), 1u);
	EXPECT_NEAR(detached.attachment("blade", "guard").rotationMatrix()[0][0], 0.0f, 0.0001f);
	EXPECT_EQ(detached.parentAttachment(), nullptr);

	auto &shoulder = character.attachment("body", "shoulder");
	auto &rebound = shoulder.attach(std::move(detached));
	EXPECT_EQ(&rebound.root(), innerRoot);
	EXPECT_EQ(rebound.parentAttachment(), &shoulder);
	EXPECT_EQ(rebound.attachment("blade", "guard").attachedCount(), 1u);
	EXPECT_NEAR(rebound.root().globalTransform()[3][0], 3.0f, 0.0001f);
}

TEST(ObjectHeadlessTest, AttachCopiesSourcePoseWithoutSharingMutableNodes)
{
	spk::Model3D::ObjectHeadless source(swordAssembly());
	auto &sourceGuard = source.attachment("blade", "guard");
	sourceGuard.setRotation(spk::Vector3{0, 0, 90});
	sourceGuard.attach(swordAssembly());
	spk::Model3D::ObjectHeadless character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &copy = grip.attach(source);

	EXPECT_NE(&copy.root(), &source.root());
	EXPECT_EQ(&copy.blueprint().root(), &source.blueprint().root());
	EXPECT_EQ(copy.attachment("blade", "guard").attachedCount(), 1u);
	EXPECT_NEAR(copy.attachment("blade", "guard").rotationMatrix()[0][0], 0.0f, 0.0001f);
	sourceGuard.setRotation(spk::Vector3{});
	EXPECT_NEAR(copy.attachment("blade", "guard").rotationMatrix()[0][0], 0.0f, 0.0001f);
	EXPECT_NEAR(sourceGuard.rotationMatrix()[0][0], 1.0f, 0.0001f);
}

TEST(ObjectHeadlessTest, CopiesNestedInstancesAndKeepsThemIndependent)
{
	spk::Model3D::ObjectHeadless original(armAssembly());
	auto &sword = original.attachment("body", "grip").attach(swordAssembly());
	auto &guard = sword.attachment("blade", "guard");
	guard.setRotation(spk::Vector3{0, 0, 45});
	guard.attach(swordAssembly());
	original.attachment("body", "shoulder").setRotation(spk::Vector3{0, 0, 90});

	spk::Model3D::ObjectHeadless copy = original;
	EXPECT_EQ(&copy.blueprint().root(), &original.blueprint().root());
	EXPECT_NE(&copy.root(), &original.root());
	EXPECT_EQ(copy.attachment("body", "grip").attachedCount(), 1u);
	EXPECT_EQ(copy.root().children().size(), original.root().children().size());
	const auto *copiedSword = dynamic_cast<const spk::Model3D::ObjectHeadless *>(copy.root().children()[1]);
	ASSERT_NE(copiedSword, nullptr);
	EXPECT_NE(copiedSword, &sword);
	EXPECT_EQ(copiedSword->attachment("blade", "guard").attachedCount(), 1u);
	EXPECT_NEAR(copiedSword->attachment("blade", "guard").rotationMatrix()[0][0], 0.7071067f, 0.0001f);
	guard.setRotation(spk::Vector3{});
	EXPECT_NEAR(copiedSword->attachment("blade", "guard").rotationMatrix()[0][0], 0.7071067f, 0.0001f);
	EXPECT_NEAR(copy.element("arm").globalTransform()[3][0], 2.0f, 0.0001f);
}

TEST(ObjectHeadlessTest, MovePreservesNodeAddressesAndOwners)
{
	spk::Model3D::ObjectHeadless original(armAssembly());
	auto &sword = original.attachment("body", "grip").attach(swordAssembly());
	auto *root = &original.root();
	auto *arm = &original.element("arm");
	auto *swordRoot = &sword.root();
	const auto transform = sword.root().globalTransform();

	spk::Model3D::ObjectHeadless moved(std::move(original));
	EXPECT_EQ(&moved.root(), root);
	EXPECT_EQ(&moved.element("arm"), arm);
	EXPECT_EQ(&moved.root().instance(), &moved);
	EXPECT_EQ(&moved.element("arm").instance(), &moved);
	EXPECT_EQ(&sword.root(), swordRoot);
	EXPECT_NEAR(sword.root().globalTransform()[3][0], transform[3][0], 0.0001f);
	EXPECT_EQ(moved.root().children().size(), 2u);
	EXPECT_THROW(original.root(), spk::Exception);

	spk::Model3D::ObjectHeadless replacement(swordAssembly());
	replacement = std::move(moved);
	EXPECT_EQ(&replacement.root(), root);
	EXPECT_EQ(&replacement.element("arm"), arm);
	EXPECT_EQ(&replacement.element("arm").instance(), &replacement);
	EXPECT_THROW(moved.root(), spk::Exception);
}

TEST(ObjectHeadlessTest, AssignmentPreservesParentAttachmentAndRecomputesPose)
{
	spk::Model3D::ObjectHeadless character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &bound = grip.attach(swordAssembly());
	const auto oldGlobal = bound.root().globalTransform();
	spk::Model3D::ObjectHeadless replacement(armAssembly());
	replacement.attachment("body", "shoulder").setRotation(spk::Vector3{0, 0, 90});

	bound = replacement;
	EXPECT_EQ(bound.parentAttachment(), &grip);
	EXPECT_EQ(&bound.root().instance(), &bound);
	EXPECT_EQ(bound.blueprint().root().name(), "body");
	EXPECT_NEAR(bound.root().globalTransform()[3][0], oldGlobal[3][0] + 1.0f, 0.0001f);
	EXPECT_NEAR(bound.element("arm").globalTransform()[3][0], 6.0f, 0.0001f);

	spk::Model3D::ObjectHeadless other(armAssembly());
	bound = std::move(other);
	EXPECT_EQ(bound.parentAttachment(), &grip);
	EXPECT_NEAR(bound.element("arm").globalTransform()[3][0], 7.0f, 0.0001f);
}

TEST(ObjectHeadlessTest, RejectsInvalidAttachmentsAndMoves)
{
	spk::Model3D::ObjectHeadless character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	EXPECT_THROW(character.attachment("body", "missing"), spk::Exception);
	EXPECT_THROW(character.element("missing"), spk::Exception);
	EXPECT_THROW(grip.detach(character), spk::Exception);
	auto &sword = grip.attach(swordAssembly());
	spk::Model3D::ObjectHeadless unrelated(swordAssembly());
	EXPECT_THROW(grip.detach(unrelated), spk::Exception);
	EXPECT_THROW(grip.attach(std::move(sword)), spk::Exception);
	EXPECT_THROW(grip.attach(std::move(character)), spk::Exception);
	EXPECT_EQ(grip.attachedCount(), 1u);
	EXPECT_EQ(sword.parentAttachment(), &grip);
	EXPECT_THROW((sword = std::move(character)), spk::Exception);
	EXPECT_EQ(sword.blueprint().root().name(), "blade");
}

TEST(ObjectHeadlessTest, RejectsQuaternionOverflowAndInvalidEulerWithoutMutation)
{
	spk::Model3D::ObjectHeadless character(armAssembly());
	auto &shoulder = character.attachment("body", "shoulder");
	shoulder.setRotation(spk::Vector3{0, 0, 90});
	const float before = character.element("arm").globalTransform()[3][0];
	const float max = std::numeric_limits<float>::max();

	EXPECT_THROW(shoulder.rotate(spk::Quaternion{max, 0, 0, max}), spk::Exception);
	EXPECT_THROW(shoulder.setRotation(spk::Vector3{0, max, std::numeric_limits<float>::quiet_NaN()}), spk::Exception);
	EXPECT_NEAR(character.element("arm").globalTransform()[3][0], before, 0.0001f);
}

TEST(ObjectHeadlessTest, ComposesStaticAttachmentOrientationBeforeDynamicRotation)
{
	spk::Model3D::Blueprint::Builder builder;
	auto &root = builder.setRoot<spk::Model3D::Blueprint::Element>("root");
	root.addAttachmentPoint("pivot", {2, 0, 0}, spk::Quaternion::fromAxisAngle({0, 0, 1}, 90.0f));
	root.attach<spk::Model3D::Blueprint::Element>("pivot", "child").setAnchor({1, 0, 0});
	spk::Model3D::ObjectHeadless instance(builder.build());
	auto &pivot = instance.attachment("root", "pivot");

	EXPECT_NEAR(instance.element("child").globalTransform()[3][0], 2.0f, 0.0001f);
	EXPECT_NEAR(instance.element("child").globalTransform()[3][1], 1.0f, 0.0001f);
	pivot.rotate(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(instance.element("child").globalTransform()[3][0], 1.0f, 0.0001f);
	EXPECT_NEAR(instance.element("child").globalTransform()[3][1], 0.0f, 0.0001f);
	EXPECT_NEAR(pivot.transform()[3][0], 2.0f, 0.0001f);
	EXPECT_NEAR(pivot.transform()[3][1], 0.0f, 0.0001f);
}

TEST(ObjectHeadlessTest, CopiesAnAttachedInstanceAsIndependentRoot)
{
	spk::Model3D::ObjectHeadless character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &original = grip.attach(swordAssembly());
	original.attachment("blade", "guard").setRotation(spk::Vector3{0, 0, 90});
	const auto attachedX = original.root().globalTransform()[3][0];

	spk::Model3D::ObjectHeadless copy = original;
	EXPECT_EQ(copy.parentAttachment(), nullptr);
	EXPECT_NE(&copy.root(), &original.root());
	EXPECT_EQ(&copy.blueprint().root(), &original.blueprint().root());
	EXPECT_NEAR(copy.root().globalTransform()[3][0], 0.0f, 0.0001f);
	EXPECT_NEAR(original.root().globalTransform()[3][0], attachedX, 0.0001f);
	EXPECT_NEAR(copy.attachment("blade", "guard").rotationMatrix()[0][0], 0.0f, 0.0001f);
	EXPECT_THROW(character.attachment("body", "shoulder").detach(original), spk::Exception);
	EXPECT_EQ(grip.attachedCount(), 1u);
}

TEST(ObjectHeadlessTest, RejectsMovingAnInstanceWhoseDataWasAlreadyTransferred)
{
	spk::Model3D::ObjectHeadless original(swordAssembly());
	spk::Model3D::ObjectHeadless transferred(std::move(original));
	spk::Model3D::ObjectHeadless destination(armAssembly());
	auto &grip = destination.attachment("body", "grip");

	EXPECT_THROW(grip.attach(std::move(original)), spk::Exception);
	EXPECT_EQ(grip.attachedCount(), 0u);
	EXPECT_THROW((transferred = std::move(original)), spk::Exception);
	EXPECT_EQ(transferred.blueprint().root().name(), "blade");
	EXPECT_EQ(&transferred.root().instance(), &transferred);
}
