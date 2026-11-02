#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <utility>

#include "exception.hpp"
#include "model.hpp"

namespace
{
	using Model = spk::Model;
	using Skeleton = Model::Skeleton;
	using Part = Skeleton::Part;
	using Matrix = spk::Matrix4x4;

	[[nodiscard]] spk::Voxel::Volume volume()
	{
		spk::Voxel::Volume::Builder builder({1, 1, 1});
		return builder.build();
	}
}

TEST(ModelCoreTest, EmptyPartsAndJointsNeedNoGraphicResources)
{
	Model model;
	EXPECT_EQ(model.skeleton().root().volume(), nullptr);
	model.skeleton().root().addJoint("hand");
	EXPECT_EQ(model.skeleton().joint("hand").child(), nullptr);
	EXPECT_EQ(model.version().topology, 1u);
}

TEST(ModelCoreTest, PartsMayCarryImmutableVoxelVolumes)
{
	Model model;
	const auto original = volume();
	model.skeleton().root().setVolume(original);
	ASSERT_NE(model.skeleton().root().volume(), nullptr);
	EXPECT_EQ(model.skeleton().root().volume()->dimensions(), original.dimensions());
	EXPECT_EQ(model.version().geometry, 1u);
	model.skeleton().root().clearVolume();
	EXPECT_EQ(model.skeleton().root().volume(), nullptr);
	EXPECT_EQ(model.version().geometry, 2u);
}

TEST(ModelCoreTest, JointRotationAndTransformIncrementPoseVersion)
{
	Model model;
	model.skeleton().root().addJoint("shoulder", Matrix::translation(2, 0, 0));
	const auto before = model.version();
	model.skeleton().joint("shoulder").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	EXPECT_GT(model.version().pose, before.pose);
	EXPECT_EQ(model.skeleton().joint("shoulder").transform()[3][0], 2.0f);
	EXPECT_THROW(model.skeleton().joint("shoulder").setRotation({0, 0, 0, 0}), spk::Exception);
	EXPECT_THROW(model.skeleton().joint("shoulder").setTransform(Matrix::scale(2, 1, 1)), spk::Exception);
	EXPECT_EQ(model.version().pose, before.pose + 1);
}

TEST(ModelCoreTest, IndependentCopiesShareVolumeDataWithoutSharingPose)
{
	Skeleton skeleton;
	skeleton.root().addJoint("arm");
	skeleton.root().joint("arm").attach(Part(volume()));
	Model first(std::move(skeleton));
	Model second(first);
	first.skeleton().joint("arm").setRotation(spk::Quaternion::fromEuler({0, 0, 45}));
	EXPECT_NE(first.skeleton().joint("arm").rotation(), second.skeleton().joint("arm").rotation());
	ASSERT_NE(second.skeleton().joint("arm").child()->volume(), nullptr);
	EXPECT_EQ(first.skeleton().joint("arm").child()->volume()->atRaw({0, 0, 0}),
		second.skeleton().joint("arm").child()->volume()->atRaw({0, 0, 0}));
	EXPECT_NE(first.version().pose, second.version().pose);
}

TEST(ModelCoreTest, NamedJointLookupRejectsAmbiguousNames)
{
	Skeleton skeleton;
	skeleton.root().addJoint("left");
	skeleton.root().addJoint("right");
	Part arm;
	arm.addJoint("elbow");
	skeleton.root().joint("left").attach(arm);
	skeleton.root().joint("right").attach(arm);
	EXPECT_THROW((void)skeleton.joint("elbow"), spk::Exception);
	EXPECT_NO_THROW((void)skeleton.joint(Skeleton::Path{"left", "elbow"}));
	EXPECT_THROW((void)skeleton.joint(Skeleton::Path{"left", "absent"}), spk::Exception);
}

TEST(ModelCoreTest, JointAttachmentRequiresKnownChildJoint)
{
	Skeleton skeleton;
	skeleton.root().addJoint("wrist");
	Part hand;
	hand.addJoint("grip");
	EXPECT_THROW(skeleton.root().joint("wrist").attach(hand, "missing"), spk::Exception);
	EXPECT_EQ(skeleton.root().joint("wrist").child(), nullptr);
	skeleton.root().joint("wrist").attach(hand, "grip");
	EXPECT_EQ(skeleton.root().joint("wrist").attachment(), "grip");
	skeleton.root().joint("wrist").detach();
	EXPECT_EQ(skeleton.root().joint("wrist").child(), nullptr);
}

TEST(ModelCoreTest, StructuralChangesAndModelTransformsAreVersionedSeparately)
{
	Model model;
	model.setTransform(Matrix::translation(5, 0, 0));
	EXPECT_EQ(model.version().transform, 1u);
	const auto pose = model.version().pose;
	model.skeleton().root().addJoint("head");
	EXPECT_EQ(model.version().topology, 1u);
	EXPECT_EQ(model.version().pose, pose);
	model.skeleton().joint("head").setRotation(spk::Quaternion::fromEuler({0, 45, 0}));
	EXPECT_EQ(model.version().topology, 1u);
	EXPECT_EQ(model.version().pose, pose + 1);
	const auto invalid = std::numeric_limits<float>::quiet_NaN();
	EXPECT_THROW(model.setTransform(Matrix::translation(invalid, 0, 0)), spk::Exception);
	EXPECT_EQ(model.version().transform, 1u);
}
