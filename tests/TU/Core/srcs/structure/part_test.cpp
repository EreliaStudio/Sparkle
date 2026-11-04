#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "exception.hpp"
#include "structure/part.hpp"

namespace
{
	class SpecializedPart : public spk::Part
	{
	public:
		int payload;

		SpecializedPart(std::string name, int value) :
			Part(std::move(name)),
			payload(value)
		{
		}
	};

	class NonMovablePart : public spk::Part
	{
	public:
		explicit NonMovablePart(std::string name) :
			Part(std::move(name))
		{
		}

		NonMovablePart(NonMovablePart &&) = delete;
	};
}

TEST(PartTest, AnchorDefaultsToIdentityAndStoresRigidTransform)
{
	spk::Part part("torso");
	EXPECT_EQ(part.anchor()[3][1], 0.0f);
	EXPECT_EQ(part.anchor()[0][0], 1.0f);

	const auto orientation = spk::Quaternion::fromAxisAngle({0, 0, 1}, 90.0f);
	part.setAnchor({1, 2, 3}, orientation);

	EXPECT_FLOAT_EQ(part.anchor()[3][0], 1.0f);
	EXPECT_FLOAT_EQ(part.anchor()[3][1], 2.0f);
	EXPECT_FLOAT_EQ(part.anchor()[3][2], 3.0f);
	EXPECT_NEAR(part.anchor()[0][1], 1.0f, 0.0001f);
}

TEST(PartTest, AttachmentConfigurationPreservesPositionAndOrientationIndependently)
{
	spk::Part part("torso");
	auto &point = part.addAttachmentPoint("shoulder");
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

TEST(PartTest, AttachmentNamesAreUniqueAndLookupThrows)
{
	spk::Part part("torso");
	part.addAttachmentPoint("shoulder");
	EXPECT_THROW(part.addAttachmentPoint("shoulder"), spk::Exception);
	EXPECT_THROW((void)part.attachment("missing"), spk::Exception);
	EXPECT_EQ(part.attachment("shoulder").name(), "shoulder");
	EXPECT_EQ(part.attachment("shoulder").child(), nullptr);
}

TEST(PartTest, AttachPreservesDerivedTypesAndSupportsInPlaceConstruction)
{
	spk::Part torso("torso");
	torso.addAttachmentPoint("left");
	torso.addAttachmentPoint("right");
	SpecializedPart left("leftArm", 17);
	auto &moved = torso.attach("left", std::move(left));
	auto &created = torso.attach<NonMovablePart>("right", "rightArm");

	EXPECT_EQ(moved.payload, 17);
	EXPECT_EQ(torso.child<SpecializedPart>("left"), &moved);
	EXPECT_EQ(torso.child<NonMovablePart>("right"), &created);
	EXPECT_THROW((void)torso.child<NonMovablePart>("left"), spk::Exception);
}

TEST(PartTest, AttachmentValidationAndRemovalKeepGeometry)
{
	spk::Part torso("torso");
	torso.addAttachmentPoint("shoulder", {3, 4, 5});
	EXPECT_THROW(torso.attach<spk::Part>("missing", "child"), spk::Exception);
	EXPECT_EQ(torso.child("shoulder"), nullptr);

	torso.attach<spk::Part>("shoulder", "child");
	EXPECT_THROW(torso.attach<spk::Part>("shoulder", "other"), spk::Exception);

	torso.removeChild("shoulder");
	EXPECT_EQ(torso.attachment("shoulder").transform()[3][0], 3.0f);
	EXPECT_EQ(torso.child("shoulder"), nullptr);
	EXPECT_THROW(torso.removeChild("shoulder"), spk::Exception);
}

TEST(PartTest, ReserveAllowsStableAttachmentReferencesWithinCapacity)
{
	spk::Part torso("torso");
	torso.reserveAttachments(3);
	auto &first = torso.addAttachmentPoint("first");
	torso.addAttachmentPoint("second");
	torso.addAttachmentPoint("third");
	first.setPosition({1, 0, 0});

	EXPECT_EQ(torso.attachment("first").transform()[3][0], 1.0f);
	EXPECT_EQ(torso.attachments().size(), 3u);
}

TEST(PartTest, MovingThroughPolymorphicBaseReferenceIsRejected)
{
	SpecializedPart derived("derived", 12);
	spk::Part &base = derived;
	spk::Part torso("torso");
	torso.addAttachmentPoint("child");
	EXPECT_THROW(torso.attach("child", std::move(base)), spk::Exception);
	EXPECT_EQ(torso.child("child"), nullptr);
}

TEST(PartTest, RejectsInvalidQuaternionWithoutChangingTransform)
{
	spk::Part torso("torso");
	auto &attachment = torso.addAttachmentPoint("point");
	EXPECT_THROW(attachment.setOrientation({0, 0, 0, 0}), spk::Exception);
	EXPECT_EQ(attachment.transform()[0][0], 1.0f);
	EXPECT_THROW(torso.setAnchor({0, 0, 0}, {0, 0, 0, 0}), spk::Exception);
	EXPECT_EQ(torso.anchor()[0][0], 1.0f);
}

TEST(PartTest, AttachmentUsesNameTrait)
{
	spk::Part part("torso");
	auto &attachment = part.addAttachmentPoint("oldName");
	attachment.setName("newName");

	EXPECT_EQ(part.attachment("newName").name(), "newName");
	EXPECT_THROW((void)part.attachment("oldName"), spk::Exception);
}
