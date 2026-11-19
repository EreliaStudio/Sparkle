#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "exception.hpp"
#include "model3d/blueprint.hpp"

namespace
{
	class SpecializedElement : public spk::Model3D::Blueprint::Element
	{
	public:
		int payload;

		SpecializedElement(std::string name, int value) :
			Element(std::move(name)),
			payload(value)
		{
		}
	};

	class NonMovableElement : public spk::Model3D::Blueprint::Element
	{
	public:
		explicit NonMovableElement(std::string name) :
			Element(std::move(name))
		{
		}

		NonMovableElement(NonMovableElement &&) = delete;
	};
}

TEST(BlueprintElementTest, AnchorDefaultsToIdentityAndStoresRigidTransform)
{
	spk::Model3D::Blueprint::Element element("torso");
	EXPECT_EQ(element.anchor()[3][1], 0.0f);
	EXPECT_EQ(element.anchor()[0][0], 1.0f);

	const auto orientation = spk::Quaternion::fromAxisAngle({0, 0, 1}, 90.0f);
	element.setAnchor({1, 2, 3}, orientation);

	EXPECT_FLOAT_EQ(element.anchor()[3][0], 1.0f);
	EXPECT_FLOAT_EQ(element.anchor()[3][1], 2.0f);
	EXPECT_FLOAT_EQ(element.anchor()[3][2], 3.0f);
	EXPECT_NEAR(element.anchor()[0][1], 1.0f, 0.0001f);
}

TEST(BlueprintElementTest, AttachmentConfigurationPreservesPositionAndOrientationIndependently)
{
	spk::Model3D::Blueprint::Element element("torso");
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

TEST(BlueprintElementTest, AttachmentNamesAreUniqueAndLookupThrows)
{
	spk::Model3D::Blueprint::Element element("torso");
	element.addAttachmentPoint("shoulder");
	EXPECT_THROW(element.addAttachmentPoint("shoulder"), spk::Exception);
	EXPECT_THROW((void)element.attachmentPoint("missing"), spk::Exception);
	EXPECT_EQ(element.attachmentPoint("shoulder").name(), "shoulder");
	EXPECT_EQ(element.attachmentPoint("shoulder").child(), nullptr);
}

TEST(BlueprintElementTest, AttachPreservesDerivedTypesAndSupportsInPlaceConstruction)
{
	spk::Model3D::Blueprint::Element torso("torso");
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

TEST(BlueprintElementTest, AttachmentValidationAndRemovalKeepGeometry)
{
	spk::Model3D::Blueprint::Element torso("torso");
	torso.addAttachmentPoint("shoulder", {3, 4, 5});
	EXPECT_THROW(torso.attach<spk::Model3D::Blueprint::Element>("missing", "child"), spk::Exception);
	EXPECT_EQ(torso.child("shoulder"), nullptr);

	torso.attach<spk::Model3D::Blueprint::Element>("shoulder", "child");
	EXPECT_THROW(torso.attach<spk::Model3D::Blueprint::Element>("shoulder", "other"), spk::Exception);

	torso.removeChild("shoulder");
	EXPECT_EQ(torso.attachmentPoint("shoulder").transform()[3][0], 3.0f);
	EXPECT_EQ(torso.child("shoulder"), nullptr);
	EXPECT_THROW(torso.removeChild("shoulder"), spk::Exception);
}

TEST(BlueprintElementTest, ReserveAllowsStableAttachmentReferencesWithinCapacity)
{
	spk::Model3D::Blueprint::Element torso("torso");
	torso.reserveAttachmentPoints(3);
	auto &first = torso.addAttachmentPoint("first");
	torso.addAttachmentPoint("second");
	torso.addAttachmentPoint("third");
	first.setPosition({1, 0, 0});

	EXPECT_EQ(torso.attachmentPoint("first").transform()[3][0], 1.0f);
	EXPECT_EQ(torso.attachmentPoints().size(), 3u);
}

TEST(BlueprintElementTest, MovingThroughPolymorphicBaseReferenceIsRejected)
{
	SpecializedElement derived("derived", 12);
	spk::Model3D::Blueprint::Element &base = derived;
	spk::Model3D::Blueprint::Element torso("torso");
	torso.addAttachmentPoint("child");
	EXPECT_THROW(torso.attach("child", std::move(base)), spk::Exception);
	EXPECT_EQ(torso.child("child"), nullptr);
}

TEST(BlueprintElementTest, RejectsInvalidQuaternionWithoutChangingTransform)
{
	spk::Model3D::Blueprint::Element torso("torso");
	auto &attachmentPoint = torso.addAttachmentPoint("point");
	EXPECT_THROW(attachmentPoint.setOrientation({0, 0, 0, 0}), spk::Exception);
	EXPECT_EQ(attachmentPoint.transform()[0][0], 1.0f);
	EXPECT_THROW(torso.setAnchor({0, 0, 0}, {0, 0, 0, 0}), spk::Exception);
	EXPECT_EQ(torso.anchor()[0][0], 1.0f);
}

TEST(BlueprintElementTest, AttachmentUsesNameTrait)
{
	spk::Model3D::Blueprint::Element element("torso");
	auto &attachmentPoint = element.addAttachmentPoint("oldName");
	attachmentPoint.setName("newName");

	EXPECT_EQ(element.attachmentPoint("newName").name(), "newName");
	EXPECT_THROW((void)element.attachmentPoint("oldName"), spk::Exception);
}
