#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <memory>
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
		builder.setRoot<spk::Assembly::Element>("blade").setAnchor({0, 2, 0});
		return builder.build();
	}
}

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

TEST(AssemblyInstanceTest, BindsInstanceAndPropagatesJointRotations)
{
	spk::Assembly::Instance character(armAssembly());
	const auto swordDefinition = swordAssembly();
	auto &grip = character.attachment("body", "grip");
	auto &sword = grip.bind(swordDefinition);
	const spk::Assembly::Attachable &node = sword;

	EXPECT_EQ(node.parentAttachment(), &grip);
	EXPECT_EQ(grip.bound(), &sword);
	EXPECT_EQ(character.root().children().size(), 2u);
	EXPECT_EQ(sword.children().front(), &sword.root());
	EXPECT_NEAR(sword.root().globalTransform()[3][0], 4.0f, 0.0001f);
	EXPECT_NEAR(sword.root().globalTransform()[3][1], 2.0f, 0.0001f);

	grip.setRotation(spk::Vector3{0, 0, 90});
	EXPECT_NEAR(sword.root().globalTransform()[3][0], 2.0f, 0.0001f);
	EXPECT_NEAR(sword.root().globalTransform()[3][1], 0.0f, 0.0001f);
}

TEST(AssemblyInstanceTest, UnbindPreservesInstanceAndResetsItsReferenceFrame)
{
	spk::Assembly::Instance character(armAssembly());
	auto &grip = character.attachment("body", "grip");
	auto &sword = grip.bind(swordAssembly());
	const auto *address = &sword;
	const float attachedX = sword.root().globalTransform()[3][0];
	EXPECT_NEAR(attachedX, 4.0f, 0.0001f);

	auto detached = grip.unbind();
	EXPECT_EQ(detached.get(), address);
	EXPECT_EQ(detached->parentAttachment(), nullptr);
	EXPECT_EQ(grip.bound(), nullptr);
	EXPECT_EQ(character.root().children().size(), 1u);
	EXPECT_NEAR(detached->root().globalTransform()[3][0], 0.0f, 0.0001f);
	EXPECT_NEAR(detached->root().globalTransform()[3][1], 2.0f, 0.0001f);
	EXPECT_EQ(&grip.bind(std::move(detached)), address);
	EXPECT_NEAR(grip.bound()->root().globalTransform()[3][0], 4.0f, 0.0001f);
}

TEST(AssemblyInstanceTest, RejectsOccupiedAndMissingAttachments)
{
	spk::Assembly::Instance character(armAssembly());
	EXPECT_THROW(character.attachment("body", "missing"), spk::Exception);
	EXPECT_THROW(character.element("missing"), spk::Exception);
	EXPECT_THROW(character.attachment("body", "shoulder").bind(swordAssembly()), spk::Exception);
	EXPECT_THROW((void)character.attachment("body", "grip").unbind(), spk::Exception);

	auto &grip = character.attachment("body", "grip");
	grip.bind(swordAssembly());
	EXPECT_THROW(grip.bind(swordAssembly()), spk::Exception);
}
