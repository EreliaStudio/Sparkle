#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

#include "exception.hpp"
#include "structure/blueprint.hpp"

namespace
{
	class TypedElement : public spk::Assembly::Blueprint::Element
	{
	public:
		int code;

		TypedElement(std::string name, int value) :
			Element(std::move(name)),
			code(value)
		{
		}
	};
	class AnotherTypedElement : public spk::Assembly::Blueprint::Element
	{
	public:
		explicit AnotherTypedElement(std::string name) :
			Element(std::move(name))
		{
		}
	};
}

TEST(BlueprintTest, SupportsPreassembledAndBuilderAssembledSubtrees)
{
	spk::Assembly::Blueprint::Element hand("leftHand");
	spk::Assembly::Blueprint::Element arm("leftArm");
	arm.addAttachmentPoint("wrist");
	arm.attach("wrist", std::move(hand));

	spk::Assembly::Blueprint::Builder builder;
	builder.setRoot<spk::Assembly::Blueprint::Element>("torso");
	builder.root().addAttachmentPoint("shoulder", {2, 0, 0});
	auto &attachedArm = builder.root().attach("shoulder", std::move(arm));
	EXPECT_EQ(attachedArm.name(), "leftArm");

	spk::Assembly::Blueprint skeleton = builder.build();
	EXPECT_EQ(skeleton.root().name(), "torso");
	EXPECT_EQ(skeleton.element("leftHand").name(), "leftHand");
	EXPECT_EQ(skeleton.element("leftArm").name(), "leftArm");
	EXPECT_EQ(skeleton.attachmentPoint("torso", "shoulder")[3][0], 2.0f);
	EXPECT_EQ(skeleton.element("torso").child("shoulder"), &skeleton.element("leftArm"));
}

TEST(BlueprintTest, SharesImmutableDataAcrossCopies)
{
	spk::Assembly::Blueprint::Builder builder;
	auto &root = builder.setRoot<TypedElement>("root", 42);
	root.addAttachmentPoint("unused");

	const spk::Assembly::Blueprint first = builder.build();
	const spk::Assembly::Blueprint second = first;

	EXPECT_EQ(&first.root(), &second.root());
	EXPECT_EQ(&first.element("root"), &second.element("root"));
	EXPECT_EQ(first.element<TypedElement>("root").code, 42);
	EXPECT_THROW((void)first.element<TypedElement>("missing"), spk::Exception);
	EXPECT_THROW((void)first.element<spk::Assembly::Blueprint::Element>("missing"), spk::Exception);
	EXPECT_THROW((void)first.element<spk::Assembly::Blueprint::Element>("unused"), spk::Exception);
}

TEST(BlueprintTest, RejectsDuplicatedNamesAndCanRetryAfterCorrection)
{
	spk::Assembly::Blueprint::Builder builder;
	auto &root = builder.setRoot<spk::Assembly::Blueprint::Element>("body");
	root.addAttachmentPoint("child");
	auto &duplicate = root.attach<spk::Assembly::Blueprint::Element>("child", "body");
	EXPECT_THROW((void)builder.build(), spk::Exception);

	duplicate.setName("renamed");
	const auto skeleton = builder.build();
	EXPECT_EQ(skeleton.element("renamed").name(), "renamed");
}

TEST(BlueprintTest, BuilderCannotBeUsedAfterBuild)
{
	spk::Assembly::Blueprint::Builder builder;
	EXPECT_THROW((void)builder.root(), spk::Exception);
	EXPECT_THROW((void)builder.build(), spk::Exception);

	builder.setRoot<spk::Assembly::Blueprint::Element>("root");
	const auto skeleton = builder.build();
	EXPECT_EQ(skeleton.root().name(), "root");
	EXPECT_THROW((void)builder.root(), spk::Exception);
	EXPECT_THROW((void)builder.build(), spk::Exception);
	EXPECT_THROW(builder.setRoot<spk::Assembly::Blueprint::Element>("other"), spk::Exception);
}

TEST(BlueprintTest, RejectsSecondRootAndWrongDerivedType)
{
	spk::Assembly::Blueprint::Builder builder;
	builder.setRoot<TypedElement>("root", 3);
	EXPECT_THROW(builder.setRoot<spk::Assembly::Blueprint::Element>("other"), spk::Exception);

	const auto skeleton = builder.build();
	EXPECT_THROW((void)skeleton.element<spk::Assembly::Blueprint::Element>("missing"), spk::Exception);
	EXPECT_THROW((void)skeleton.element<TypedElement>("wrong"), spk::Exception);
	EXPECT_THROW((void)skeleton.element<AnotherTypedElement>("root"), spk::Exception);
}

TEST(BlueprintTest, RejectsRootMoveThroughBaseReference)
{
	TypedElement typed("root", 9);
	spk::Assembly::Blueprint::Element &base = typed;
	spk::Assembly::Blueprint::Builder builder;

	EXPECT_THROW(builder.setRoot(std::move(base)), spk::Exception);
	auto &root = builder.setRoot<TypedElement>("root", 10);
	EXPECT_EQ(root.code, 10);
	EXPECT_EQ(builder.build().element<TypedElement>("root").code, 10);
}

TEST(BlueprintTest, AcceptsMovedRootAndPreservesItsDerivedType)
{
	TypedElement root("root", 27);
	root.addAttachmentPoint("child");
	root.attach<spk::Assembly::Blueprint::Element>("child", "leaf");

	spk::Assembly::Blueprint::Builder builder;
	builder.setRoot(std::move(root));
	const auto skeleton = builder.build();

	EXPECT_EQ(skeleton.element<TypedElement>("root").code, 27);
	EXPECT_EQ(skeleton.element("leaf").name(), "leaf");
}

TEST(BlueprintTest, RejectsDuplicatedAttachmentNamesAfterRenaming)
{
	spk::Assembly::Blueprint::Builder builder;
	auto &root = builder.setRoot<spk::Assembly::Blueprint::Element>("root");
	root.addAttachmentPoint("first");
	root.addAttachmentPoint("second").setName("first");

	EXPECT_THROW((void)builder.build(), spk::Exception);
}
