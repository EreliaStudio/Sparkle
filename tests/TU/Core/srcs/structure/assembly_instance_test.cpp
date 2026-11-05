#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include "exception.hpp"
#include "structure/assembly.hpp"

namespace
{
	spk::Assembly armAssembly()
	{
		spk::Assembly::Builder builder;
		auto &body = builder.setRoot<spk::Assembly::Element>("body");
		body.setAnchor({1, 0, 0});
		body.addAttachmentPoint("shoulder", {2, 0, 0});
		auto &arm = body.attach<spk::Assembly::Element>("shoulder", "arm");
		arm.setAnchor({0, 1, 0});
		arm.addAttachmentPoint("wrist", {1, 0, 0});
		arm.attach<spk::Assembly::Element>("wrist", "hand").setAnchor({1, 0, 0});
		body.addAttachmentPoint("grip", {3, 0, 0});
		return builder.build();
	}

	spk::Assembly swordAssembly()
	{
		spk::Assembly::Builder builder;
		auto &blade = builder.setRoot<spk::Assembly::Element>("blade");
		blade.setAnchor({0, 2, 0});
		blade.addAttachmentPoint("guard", {0, 1, 0});
		return builder.build();
	}
}

static_assert(std::is_same_v<
	decltype(std::declval<const spk::Assembly::Attachable &>().children()),
	const std::vector<const spk::Assembly::Attachable *> &>);

TEST(AssemblyInstanceTest, UsesSharedDefinitionsAndIndependentGlobalTransforms)
{
	const auto definition = armAssembly();
	spk::Assembly::Instance first(definition);
	spk::Assembly::Instance second(definition);

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

TEST(AssemblyInstanceTest, RotatesLocallyAndKeepsJointPivotFixed)
{
	spk::Assembly::Instance pose(armAssembly());
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

TEST(AssemblyInstanceTest, RejectsInvalidRotationsWithoutChangingPose)
{
	spk::Assembly::Instance pose(armAssembly());
	auto &shoulder = pose.attachment("body", "shoulder");
	shoulder.setRotation(spk::Vector3{0, 0, 90});
	const float before = pose.element("arm").globalTransform()[3][0];

	EXPECT_THROW(shoulder.setRotation(spk::Quaternion{0, 0, 0, 0}), spk::Exception);
	EXPECT_THROW(shoulder.rotate(spk::Vector3{std::numeric_limits<float>::infinity(), 0, 0}), spk::Exception);
	EXPECT_NEAR(pose.element("arm").globalTransform()[3][0], before, 0.0001f);
}

TEST(AssemblyInstanceTest, TraversesOnlyDirectChildrenInStructuralThenInsertionOrder)
{
	spk::Assembly::Instance character(armAssembly());
	auto &shoulder = character.attachment("body", "shoulder");
	auto &grip = character.attachment("body", "grip");
	auto &sword = grip.bind(swordAssembly());
	auto &shield = shoulder.bind(swordAssembly());
	auto &dagger = grip.bind(swordAssembly());

	ASSERT_EQ(character.children().size(), 1u);
	EXPECT_EQ(character.children()[0], &character.root());
	ASSERT_EQ(character.root().children().size(), 4u);
	EXPECT_EQ(character.root().children()[0], &character.element("arm"));
	EXPECT_EQ(character.root().children()[1], &sword);
	EXPECT_EQ(character.root().children()[2], &shield);
	EXPECT_EQ(character.root().children()[3], &dagger);
	EXPECT_EQ(shoulder.boundCount(), 1u);
	EXPECT_EQ(grip.boundCount(), 2u);
	EXPECT_EQ(sword.children()[0], &sword.root());
	EXPECT_EQ(character.element("arm").children()[0], &character.element("hand"));
}

TEST(AssemblyInstanceTest, BindsMultipleAccessoriesToStructuralAttachment)
{
	spk::Assembly::Instance character(armAssembly());
	auto &shoulder = character.attachment("body", "shoulder");
	auto &sword = shoulder.bind(swordAssembly());
	auto &shield = shoulder.bind(swordAssembly());
	const auto *swordRoot = &sword.root();

	EXPECT_EQ(shoulder.boundCount(), 2u);
	EXPECT_EQ(sword.parentAttachment(), &shoulder);
	EXPECT_EQ(shield.parentAttachment(), &shoulder);
	EXPECT_EQ(character.root().children()[0], &character.element("arm"));
	shoulder.setRotation(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(sword.root().globalTransform()[3][0], 1.0f, 0.0001f);
	EXPECT_NEAR(sword.root().globalTransform()[3][1], 0.0f, 0.0001f);
	EXPECT_NEAR(shield.root().globalTransform()[3][0], 1.0f, 0.0001f);
	EXPECT_EQ(&sword.root(), swordRoot);
}

TEST(AssemblyInstanceTest, InvalidatesOnlyArticulatedSubtreeAndPreservesOtherBranch)
{
	spk::Assembly::Instance character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &sword = grip.bind(swordAssembly());
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

TEST(AssemblyInstanceTest, ReusesCachedTransformUntilPoseChanges)
{
	spk::Assembly::Instance character(armAssembly());
	const auto &first = character.element("arm").globalTransform();
	const auto &second = character.element("arm").globalTransform();
	EXPECT_EQ(&first, &second);
	EXPECT_NEAR(first[3][0], 3.0f, 0.0001f);

	character.attachment("body", "shoulder").setRotation(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(character.element("arm").globalTransform()[3][0], 2.0f, 0.0001f);
}

TEST(AssemblyInstanceTest, DetachesSpecificAccessoryWithoutAffectingOthers)
{
	spk::Assembly::Instance character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &sword = grip.bind(swordAssembly());
	auto &shield = grip.bind(swordAssembly());
	const auto *originalRoot = &sword.root();
	const auto *otherRoot = &shield.root();
	grip.rotate(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(sword.root().globalTransform()[3][0], 2.0f, 0.0001f);

	spk::Assembly::Instance detached = grip.unbind(sword);
	EXPECT_EQ(detached.parentAttachment(), nullptr);
	EXPECT_EQ(&detached.root(), originalRoot);
	EXPECT_EQ(grip.boundCount(), 1u);
	EXPECT_EQ(&shield.root(), otherRoot);
	EXPECT_EQ(shield.parentAttachment(), &grip);
	EXPECT_NEAR(detached.root().globalTransform()[3][0], 0.0f, 0.0001f);
	EXPECT_NEAR(detached.root().globalTransform()[3][1], 2.0f, 0.0001f);
	EXPECT_EQ(character.root().children().size(), 2u);
}

TEST(AssemblyInstanceTest, DetachThenRebindKeepsRecursiveArticulations)
{
	spk::Assembly::Instance character(armAssembly());
	spk::Assembly::Instance sword(swordAssembly());
	sword.attachment("blade", "guard").setRotation(spk::Vector3{0, 0, 90});
	auto &gem = sword.attachment("blade", "guard").bind(swordAssembly());
	gem.attachment("blade", "guard").rotate(spk::Vector3{0, 0, 45});

	auto &grip = character.attachment("body", "grip");
	auto &attached = grip.bind(std::move(sword));
	const auto *innerRoot = &attached.root();
	spk::Assembly::Instance detached = grip.unbind(attached);
	EXPECT_EQ(detached.attachment("blade", "guard").boundCount(), 1u);
	EXPECT_NEAR(detached.attachment("blade", "guard").rotationMatrix()[0][0], 0.0f, 0.0001f);
	EXPECT_EQ(detached.parentAttachment(), nullptr);

	auto &shoulder = character.attachment("body", "shoulder");
	auto &rebound = shoulder.bind(std::move(detached));
	EXPECT_EQ(&rebound.root(), innerRoot);
	EXPECT_EQ(rebound.parentAttachment(), &shoulder);
	EXPECT_EQ(rebound.attachment("blade", "guard").boundCount(), 1u);
	EXPECT_NEAR(rebound.root().globalTransform()[3][0], 3.0f, 0.0001f);
}

TEST(AssemblyInstanceTest, BindCopiesSourcePoseWithoutSharingMutableNodes)
{
	spk::Assembly::Instance source(swordAssembly());
	auto &sourceGuard = source.attachment("blade", "guard");
	sourceGuard.setRotation(spk::Vector3{0, 0, 90});
	sourceGuard.bind(swordAssembly());
	spk::Assembly::Instance character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &copy = grip.bind(source);

	EXPECT_NE(&copy.root(), &source.root());
	EXPECT_EQ(&copy.assembly().root(), &source.assembly().root());
	EXPECT_EQ(copy.attachment("blade", "guard").boundCount(), 1u);
	EXPECT_NEAR(copy.attachment("blade", "guard").rotationMatrix()[0][0], 0.0f, 0.0001f);
	sourceGuard.setRotation(spk::Vector3{});
	EXPECT_NEAR(copy.attachment("blade", "guard").rotationMatrix()[0][0], 0.0f, 0.0001f);
	EXPECT_NEAR(sourceGuard.rotationMatrix()[0][0], 1.0f, 0.0001f);
}

TEST(AssemblyInstanceTest, CopiesNestedInstancesAndKeepsThemIndependent)
{
	spk::Assembly::Instance original(armAssembly());
	auto &sword = original.attachment("body", "grip").bind(swordAssembly());
	auto &guard = sword.attachment("blade", "guard");
	guard.setRotation(spk::Vector3{0, 0, 45});
	guard.bind(swordAssembly());
	original.attachment("body", "shoulder").setRotation(spk::Vector3{0, 0, 90});

	spk::Assembly::Instance copy = original;
	EXPECT_EQ(&copy.assembly().root(), &original.assembly().root());
	EXPECT_NE(&copy.root(), &original.root());
	EXPECT_EQ(copy.attachment("body", "grip").boundCount(), 1u);
	EXPECT_EQ(copy.root().children().size(), original.root().children().size());
	const auto *copiedSword = dynamic_cast<const spk::Assembly::Instance *>(copy.root().children()[1]);
	ASSERT_NE(copiedSword, nullptr);
	EXPECT_NE(copiedSword, &sword);
	EXPECT_EQ(copiedSword->attachment("blade", "guard").boundCount(), 1u);
	EXPECT_NEAR(copiedSword->attachment("blade", "guard").rotationMatrix()[0][0], 0.7071067f, 0.0001f);
	guard.setRotation(spk::Vector3{});
	EXPECT_NEAR(copiedSword->attachment("blade", "guard").rotationMatrix()[0][0], 0.7071067f, 0.0001f);
	EXPECT_NEAR(copy.element("arm").globalTransform()[3][0], 2.0f, 0.0001f);
}

TEST(AssemblyInstanceTest, MovePreservesNodeAddressesAndOwners)
{
	spk::Assembly::Instance original(armAssembly());
	auto &sword = original.attachment("body", "grip").bind(swordAssembly());
	auto *root = &original.root();
	auto *arm = &original.element("arm");
	auto *swordRoot = &sword.root();
	const auto transform = sword.root().globalTransform();

	spk::Assembly::Instance moved(std::move(original));
	EXPECT_EQ(&moved.root(), root);
	EXPECT_EQ(&moved.element("arm"), arm);
	EXPECT_EQ(&moved.root().instance(), &moved);
	EXPECT_EQ(&moved.element("arm").instance(), &moved);
	EXPECT_EQ(&sword.root(), swordRoot);
	EXPECT_NEAR(sword.root().globalTransform()[3][0], transform[3][0], 0.0001f);
	EXPECT_EQ(moved.root().children().size(), 2u);
	EXPECT_THROW(original.root(), spk::Exception);

	spk::Assembly::Instance replacement(swordAssembly());
	replacement = std::move(moved);
	EXPECT_EQ(&replacement.root(), root);
	EXPECT_EQ(&replacement.element("arm"), arm);
	EXPECT_EQ(&replacement.element("arm").instance(), &replacement);
	EXPECT_THROW(moved.root(), spk::Exception);
}

TEST(AssemblyInstanceTest, AssignmentPreservesParentAttachmentAndRecomputesPose)
{
	spk::Assembly::Instance character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &bound = grip.bind(swordAssembly());
	const auto oldGlobal = bound.root().globalTransform();
	spk::Assembly::Instance replacement(armAssembly());
	replacement.attachment("body", "shoulder").setRotation(spk::Vector3{0, 0, 90});

	bound = replacement;
	EXPECT_EQ(bound.parentAttachment(), &grip);
	EXPECT_EQ(&bound.root().instance(), &bound);
	EXPECT_EQ(bound.assembly().root().name(), "body");
	EXPECT_NEAR(bound.root().globalTransform()[3][0], oldGlobal[3][0] + 1.0f, 0.0001f);
	EXPECT_NEAR(bound.element("arm").globalTransform()[3][0], 6.0f, 0.0001f);

	spk::Assembly::Instance other(armAssembly());
	bound = std::move(other);
	EXPECT_EQ(bound.parentAttachment(), &grip);
	EXPECT_NEAR(bound.element("arm").globalTransform()[3][0], 7.0f, 0.0001f);
}

TEST(AssemblyInstanceTest, RejectsInvalidAttachmentsAndMoves)
{
	spk::Assembly::Instance character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	EXPECT_THROW(character.attachment("body", "missing"), spk::Exception);
	EXPECT_THROW(character.element("missing"), spk::Exception);
	EXPECT_THROW(grip.unbind(character), spk::Exception);
	auto &sword = grip.bind(swordAssembly());
	spk::Assembly::Instance unrelated(swordAssembly());
	EXPECT_THROW(grip.unbind(unrelated), spk::Exception);
	EXPECT_THROW(grip.bind(std::move(sword)), spk::Exception);
	EXPECT_THROW(grip.bind(std::move(character)), spk::Exception);
	EXPECT_EQ(grip.boundCount(), 1u);
	EXPECT_EQ(sword.parentAttachment(), &grip);
	EXPECT_THROW((sword = std::move(character)), spk::Exception);
	EXPECT_EQ(sword.assembly().root().name(), "blade");
}

TEST(AssemblyInstanceTest, RejectsQuaternionOverflowAndInvalidEulerWithoutMutation)
{
	spk::Assembly::Instance character(armAssembly());
	auto &shoulder = character.attachment("body", "shoulder");
	shoulder.setRotation(spk::Vector3{0, 0, 90});
	const float before = character.element("arm").globalTransform()[3][0];
	const float max = std::numeric_limits<float>::max();

	EXPECT_THROW(shoulder.rotate(spk::Quaternion{max, 0, 0, max}), spk::Exception);
	EXPECT_THROW(shoulder.setRotation(spk::Vector3{0, max, std::numeric_limits<float>::quiet_NaN()}), spk::Exception);
	EXPECT_NEAR(character.element("arm").globalTransform()[3][0], before, 0.0001f);
}

TEST(AssemblyInstanceTest, ComposesStaticAttachmentOrientationBeforeDynamicRotation)
{
	spk::Assembly::Builder builder;
	auto &root = builder.setRoot<spk::Assembly::Element>("root");
	root.addAttachmentPoint("pivot", {2, 0, 0}, spk::Quaternion::fromAxisAngle({0, 0, 1}, 90.0f));
	root.attach<spk::Assembly::Element>("pivot", "child").setAnchor({1, 0, 0});
	spk::Assembly::Instance instance(builder.build());
	auto &pivot = instance.attachment("root", "pivot");

	EXPECT_NEAR(instance.element("child").globalTransform()[3][0], 2.0f, 0.0001f);
	EXPECT_NEAR(instance.element("child").globalTransform()[3][1], 1.0f, 0.0001f);
	pivot.rotate(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(instance.element("child").globalTransform()[3][0], 1.0f, 0.0001f);
	EXPECT_NEAR(instance.element("child").globalTransform()[3][1], 0.0f, 0.0001f);
	EXPECT_NEAR(pivot.transform()[3][0], 2.0f, 0.0001f);
	EXPECT_NEAR(pivot.transform()[3][1], 0.0f, 0.0001f);
}

TEST(AssemblyInstanceTest, CopiesAnAttachedInstanceAsIndependentRoot)
{
	spk::Assembly::Instance character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &original = grip.bind(swordAssembly());
	original.attachment("blade", "guard").setRotation(spk::Vector3{0, 0, 90});
	const auto attachedX = original.root().globalTransform()[3][0];

	spk::Assembly::Instance copy = original;
	EXPECT_EQ(copy.parentAttachment(), nullptr);
	EXPECT_NE(&copy.root(), &original.root());
	EXPECT_EQ(&copy.assembly().root(), &original.assembly().root());
	EXPECT_NEAR(copy.root().globalTransform()[3][0], 0.0f, 0.0001f);
	EXPECT_NEAR(original.root().globalTransform()[3][0], attachedX, 0.0001f);
	EXPECT_NEAR(copy.attachment("blade", "guard").rotationMatrix()[0][0], 0.0f, 0.0001f);
	EXPECT_THROW(character.attachment("body", "shoulder").unbind(original), spk::Exception);
	EXPECT_EQ(grip.boundCount(), 1u);
}

TEST(AssemblyInstanceTest, RejectsMovingAnInstanceWhoseDataWasAlreadyTransferred)
{
	spk::Assembly::Instance original(swordAssembly());
	spk::Assembly::Instance transferred(std::move(original));
	spk::Assembly::Instance destination(armAssembly());
	auto &grip = destination.attachment("body", "grip");

	EXPECT_THROW(grip.bind(std::move(original)), spk::Exception);
	EXPECT_EQ(grip.boundCount(), 0u);
	EXPECT_THROW((transferred = std::move(original)), spk::Exception);
	EXPECT_EQ(transferred.assembly().root().name(), "blade");
	EXPECT_EQ(&transferred.root().instance(), &transferred);
}
