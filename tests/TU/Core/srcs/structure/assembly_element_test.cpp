#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "exception.hpp"
#include "structure/assembly.hpp"

namespace
{
	class SpecializedElement : public spk::Assembly::Element
	{
	public:
		int payload;

		SpecializedElement(std::string name, int value) :
			Element(std::move(name)),
			payload(value)
		{
		}
	};

	class NonMovableElement : public spk::Assembly::Element
	{
	public:
		explicit NonMovableElement(std::string name) :
			Element(std::move(name))
		{
		}

		NonMovableElement(NonMovableElement &&) = delete;
	};
}

TEST(AssemblyElementTest, AnchorDefaultsToIdentityAndStoresRigidTransform)
{
	spk::Assembly::Element element("torso");
	EXPECT_EQ(element.anchor()[3][1], 0.0f);
	EXPECT_EQ(element.anchor()[0][0], 1.0f);

	const auto orientation = spk::Quaternion::fromAxisAngle({0, 0, 1}, 90.0f);
	element.setAnchor({1, 2, 3}, orientation);

	EXPECT_FLOAT_EQ(element.anchor()[3][0], 1.0f);
	EXPECT_FLOAT_EQ(element.anchor()[3][1], 2.0f);
	EXPECT_FLOAT_EQ(element.anchor()[3][2], 3.0f);
	EXPECT_NEAR(element.anchor()[0][1], 1.0f, 0.0001f);
}

TEST(AssemblyElementTest, AttachmentConfigurationPreservesPositionAndOrientationIndependently)
{
	spk::Assembly::Element element("torso");
	auto &point = element.addAttachmentPoint("shoulder");
	point.setPosition({2, 3, 4});
	EXPECT_EQ(point.transform()[3][0], 2.0f);

	const auto orientation = spk::Quaternion::fromAxisAngle({0, 1, 0}, 90.0f);
	point.setOrientation(orientation);
	EXPECT_FLOAT_EQ(point.transform()[3][1], 3.0f);
	EXPECT_NEAR(point.transform()[0][2], -1.0f, 0.0001f);

	point.setTransform({5, 6, 7});
	EXPECT_FLOAT_EQ(point.transform()[3][2], 7.0f);
	EXPECT_FLOAT_EQ(point.transform()[0][0], 1.0f);
}

TEST(AssemblyElementTest, AttachmentNamesAreUniqueAndLookupThrows)
{
	spk::Assembly::Element element("torso");
	element.addAttachmentPoint("shoulder");
	EXPECT_THROW(element.addAttachmentPoint("shoulder"), spk::Exception);
	EXPECT_THROW((void)element.attachment("missing"), spk::Exception);
	EXPECT_EQ(element.attachment("shoulder").name(), "shoulder");
	EXPECT_EQ(element.attachment("shoulder").child(), nullptr);
}

TEST(AssemblyElementTest, AttachPreservesDerivedTypesAndSupportsInPlaceConstruction)
{
	spk::Assembly::Element torso("torso");
	torso.addAttachmentPoint("left");
	torso.addAttachmentPoint("right");
	SpecializedElement left("leftArm", 17);
	auto &moved = torso.attach("left", std::move(left));
	auto &created = torso.attach<NonMovableElement>("right", "rightArm");

	EXPECT_EQ(moved.payload, 17);
	EXPECT_EQ(torso.child<SpecializedElement>("left"), &moved);
	EXPECT_EQ(torso.child<NonMovableElement>("right"), &created);
	EXPECT_THROW((void)torso.child<NonMovableElement>("left"), spk::Exception);
}

TEST(AssemblyElementTest, AttachmentValidationAndRemovalKeepGeometry)
{
	spk::Assembly::Element torso("torso");
	torso.addAttachmentPoint("shoulder", {3, 4, 5});
	EXPECT_THROW(torso.attach<spk::Assembly::Element>("missing", "child"), spk::Exception);
	EXPECT_EQ(torso.child("shoulder"), nullptr);

	torso.attach<spk::Assembly::Element>("shoulder", "child");
	EXPECT_THROW(torso.attach<spk::Assembly::Element>("shoulder", "other"), spk::Exception);

	torso.removeChild("shoulder");
	EXPECT_EQ(torso.attachment("shoulder").transform()[3][0], 3.0f);
	EXPECT_EQ(torso.child("shoulder"), nullptr);
	EXPECT_THROW(torso.removeChild("shoulder"), spk::Exception);
}

TEST(AssemblyElementTest, ReserveAllowsStableAttachmentReferencesWithinCapacity)
{
	spk::Assembly::Element torso("torso");
	torso.reserveAttachments(3);
	auto &first = torso.addAttachmentPoint("first");
	torso.addAttachmentPoint("second");
	torso.addAttachmentPoint("third");
	first.setPosition({1, 0, 0});

	EXPECT_EQ(torso.attachment("first").transform()[3][0], 1.0f);
	EXPECT_EQ(torso.attachments().size(), 3u);
}

TEST(AssemblyElementTest, MovingThroughPolymorphicBaseReferenceIsRejected)
{
	SpecializedElement derived("derived", 12);
	spk::Assembly::Element &base = derived;
	spk::Assembly::Element torso("torso");
	torso.addAttachmentPoint("child");
	EXPECT_THROW(torso.attach("child", std::move(base)), spk::Exception);
	EXPECT_EQ(torso.child("child"), nullptr);
}

TEST(AssemblyElementTest, RejectsInvalidQuaternionWithoutChangingTransform)
{
	spk::Assembly::Element torso("torso");
	auto &attachment = torso.addAttachmentPoint("point");
	EXPECT_THROW(attachment.setOrientation({0, 0, 0, 0}), spk::Exception);
	EXPECT_EQ(attachment.transform()[0][0], 1.0f);
	EXPECT_THROW(torso.setAnchor({0, 0, 0}, {0, 0, 0, 0}), spk::Exception);
	EXPECT_EQ(torso.anchor()[0][0], 1.0f);
}

TEST(AssemblyElementTest, AttachmentUsesNameTrait)
{
	spk::Assembly::Element element("torso");
	auto &attachment = element.addAttachmentPoint("oldName");
	attachment.setName("newName");

	EXPECT_EQ(element.attachment("newName").name(), "newName");
	EXPECT_THROW((void)element.attachment("oldName"), spk::Exception);
}
