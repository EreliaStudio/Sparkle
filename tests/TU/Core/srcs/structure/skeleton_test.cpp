#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

#include "exception.hpp"
#include "structure/skeleton.hpp"

namespace
{
	class TypedPart : public spk::Part
	{
	public:
		int code;

		TypedPart(std::string name, int value) :
			Part(std::move(name)),
			code(value)
		{
		}
	};
	class AnotherTypedPart : public spk::Part
	{
	public:
		explicit AnotherTypedPart(std::string name) :
			Part(std::move(name))
		{
		}
	};
}

TEST(SkeletonTest, SupportsPreassembledAndBuilderAssembledSubtrees)
{
	spk::Part hand("leftHand");
	spk::Part arm("leftArm");
	arm.addAttachmentPoint("wrist");
	arm.attach("wrist", std::move(hand));

	spk::Skeleton::Builder builder;
	builder.setRoot<spk::Part>("torso");
	builder.root().addAttachmentPoint("shoulder", {2, 0, 0});
	auto &attachedArm = builder.root().attach("shoulder", std::move(arm));
	EXPECT_EQ(attachedArm.name(), "leftArm");

	spk::Skeleton skeleton = builder.build();
	EXPECT_EQ(skeleton.root().name(), "torso");
	EXPECT_EQ(skeleton.part("leftHand").name(), "leftHand");
	EXPECT_EQ(skeleton.part("leftArm").name(), "leftArm");
	EXPECT_EQ(skeleton.attachmentPoint("torso", "shoulder")[3][0], 2.0f);
	EXPECT_EQ(skeleton.part("torso").child("shoulder"), &skeleton.part("leftArm"));
}

TEST(SkeletonTest, SharesImmutableDataAcrossCopies)
{
	spk::Skeleton::Builder builder;
	auto &root = builder.setRoot<TypedPart>("root", 42);
	root.addAttachmentPoint("unused");

	const spk::Skeleton first = builder.build();
	const spk::Skeleton second = first;

	EXPECT_EQ(&first.root(), &second.root());
	EXPECT_EQ(&first.part("root"), &second.part("root"));
	EXPECT_EQ(first.part<TypedPart>("root").code, 42);
	EXPECT_THROW((void)first.part<TypedPart>("missing"), spk::Exception);
	EXPECT_THROW((void)first.part<spk::Part>("missing"), spk::Exception);
	EXPECT_THROW((void)first.part<spk::Part>("unused"), spk::Exception);
}

TEST(SkeletonTest, RejectsDuplicatedNamesAndCanRetryAfterCorrection)
{
	spk::Skeleton::Builder builder;
	auto &root = builder.setRoot<spk::Part>("body");
	root.addAttachmentPoint("child");
	auto &duplicate = root.attach<spk::Part>("child", "body");
	EXPECT_THROW((void)builder.build(), spk::Exception);

	duplicate.setName("renamed");
	const auto skeleton = builder.build();
	EXPECT_EQ(skeleton.part("renamed").name(), "renamed");
}

TEST(SkeletonTest, BuilderCannotBeUsedAfterBuild)
{
	spk::Skeleton::Builder builder;
	EXPECT_THROW((void)builder.root(), spk::Exception);
	EXPECT_THROW((void)builder.build(), spk::Exception);

	builder.setRoot<spk::Part>("root");
	const auto skeleton = builder.build();
	EXPECT_EQ(skeleton.root().name(), "root");
	EXPECT_THROW((void)builder.root(), spk::Exception);
	EXPECT_THROW((void)builder.build(), spk::Exception);
	EXPECT_THROW(builder.setRoot<spk::Part>("other"), spk::Exception);
}

TEST(SkeletonTest, RejectsSecondRootAndWrongDerivedType)
{
	spk::Skeleton::Builder builder;
	builder.setRoot<TypedPart>("root", 3);
	EXPECT_THROW(builder.setRoot<spk::Part>("other"), spk::Exception);

	const auto skeleton = builder.build();
	EXPECT_THROW((void)skeleton.part<spk::Part>("missing"), spk::Exception);
	EXPECT_THROW((void)skeleton.part<TypedPart>("wrong"), spk::Exception);
	EXPECT_THROW((void)skeleton.part<AnotherTypedPart>("root"), spk::Exception);
}

TEST(SkeletonTest, RejectsRootMoveThroughBaseReference)
{
	TypedPart typed("root", 9);
	spk::Part &base = typed;
	spk::Skeleton::Builder builder;

	EXPECT_THROW(builder.setRoot(std::move(base)), spk::Exception);
	auto &root = builder.setRoot<TypedPart>("root", 10);
	EXPECT_EQ(root.code, 10);
	EXPECT_EQ(builder.build().part<TypedPart>("root").code, 10);
}

TEST(SkeletonTest, AcceptsMovedRootAndPreservesItsDerivedType)
{
	TypedPart root("root", 27);
	root.addAttachmentPoint("child");
	root.attach<spk::Part>("child", "leaf");

	spk::Skeleton::Builder builder;
	builder.setRoot(std::move(root));
	const auto skeleton = builder.build();

	EXPECT_EQ(skeleton.part<TypedPart>("root").code, 27);
	EXPECT_EQ(skeleton.part("leaf").name(), "leaf");
}

TEST(SkeletonTest, RejectsDuplicatedAttachmentNamesAfterRenaming)
{
	spk::Skeleton::Builder builder;
	auto &root = builder.setRoot<spk::Part>("root");
	root.addAttachmentPoint("first");
	root.addAttachmentPoint("second").setName("first");

	EXPECT_THROW((void)builder.build(), spk::Exception);
}
