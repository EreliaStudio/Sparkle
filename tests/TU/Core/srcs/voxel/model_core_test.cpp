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
	const auto firstCell = first.skeleton().joint("arm").child()->volume()->atRaw({0, 0, 0});
	const auto secondCell = second.skeleton().joint("arm").child()->volume()->atRaw({0, 0, 0});
	EXPECT_EQ(firstCell, secondCell);
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

TEST(ModelCoreTest, ReplacingAnExistingJointInvalidatesTopology)
{
	Model model;
	model.skeleton().root().addJoint("arm");
	const auto before = model.version();
	Skeleton::Joint replacement;
	replacement.attach(Part(volume()));
	model.skeleton().joint("arm") = std::move(replacement);
	EXPECT_GT(model.version().topology, before.topology);
	ASSERT_NE(model.skeleton().joint("arm").child(), nullptr);
	EXPECT_NE(model.skeleton().joint("arm").child()->volume(), nullptr);
}

TEST(ModelCoreTest, JointCanAttachIndependentModelWithNamedGrip)
{
	Model character;
	character.skeleton().root().addJoint("hand");
	Model sword;
	sword.skeleton().root().setVolume(volume());
	sword.skeleton().root().addJoint("grip", Matrix::translation(1, 0, 0));
	const auto before = character.version().topology;
	auto &hand = character.skeleton().joint("hand");
	EXPECT_THROW(hand.attach(sword, "missing"), spk::Exception);
	EXPECT_EQ(hand.model(), nullptr);
	EXPECT_EQ(character.version().topology, before);
	hand.attach(sword, "grip");
	ASSERT_NE(hand.model(), nullptr);
	EXPECT_NE(hand.model(), &sword);
	EXPECT_EQ(hand.child(), nullptr);
	EXPECT_EQ(hand.attachment(), "grip");
	EXPECT_GT(character.version().topology, before);
	EXPECT_NE(hand.model()->skeleton().root().volume(), nullptr);
	hand.model()->setTransform(Matrix::translation(0, 2, 0));
	EXPECT_EQ(sword.transform()[3][1], 0.0f);
}

TEST(ModelCoreTest, JointSwitchesBetweenPartAndModelAndDetaches)
{
	Model character;
	character.skeleton().root().addJoint("hand");
	auto &hand = character.skeleton().joint("hand");
	hand.attach(Part(volume()));
	ASSERT_NE(hand.child(), nullptr);
	const auto firstVersion = hand.attachmentVersion();
	hand.attach(Model{});
	EXPECT_EQ(hand.child(), nullptr);
	ASSERT_NE(hand.model(), nullptr);
	EXPECT_GT(hand.attachmentVersion(), firstVersion);
	hand.attach(Part{});
	EXPECT_EQ(hand.model(), nullptr);
	ASSERT_NE(hand.child(), nullptr);
	hand.detach();
	EXPECT_EQ(hand.child(), nullptr);
	EXPECT_EQ(hand.model(), nullptr);
	EXPECT_TRUE(hand.attachment().empty());
}

TEST(ModelCoreTest, AttachedModelIsDeepCopiedWithItsOwnPose)
{
	Model character;
	character.skeleton().root().addJoint("hand");
	Model weapon;
	weapon.skeleton().root().addJoint("trigger");
	character.skeleton().joint("hand").attach(weapon);
	Model clone(character);
	auto *original = character.skeleton().joint("hand").model();
	auto *independent = clone.skeleton().joint("hand").model();
	ASSERT_NE(original, nullptr);
	ASSERT_NE(independent, nullptr);
	EXPECT_NE(original, independent);
	original->skeleton().joint("trigger").setRotation(spk::Quaternion::fromEuler({0, 0, 30}));
	EXPECT_NE(original->skeleton().joint("trigger").rotation(), independent->skeleton().joint("trigger").rotation());
	EXPECT_EQ(clone.skeleton().joint("hand").child(), nullptr);
}

TEST(ModelCoreTest, AttachedModelRevisionsStayLocal)
{
	Model parent;
	parent.skeleton().root().addJoint("hand");
	Model child;
	child.skeleton().root().addJoint("tip");
	parent.skeleton().joint("hand").attach(std::move(child));
	const auto before = parent.version();
	auto *attached = parent.skeleton().joint("hand").model();
	ASSERT_NE(attached, nullptr);
	attached->setTransform(Matrix::translation(1, 2, 3));
	attached->skeleton().joint("tip").setRotation(spk::Quaternion::fromEuler({0, 0, 30}));
	attached->skeleton().root().setVolume(volume());
	attached->skeleton().root().addJoint("extra");
	EXPECT_EQ(parent.version(), before);
	EXPECT_EQ(attached->version().transform, 1u);
	EXPECT_EQ(attached->version().pose, 1u);
	EXPECT_EQ(attached->version().geometry, 1u);
	EXPECT_EQ(attached->version().topology, 2u);
}

TEST(ModelCoreTest, CachedGlobalMatricesComposeNestedJointsAndPartAnchors)
{
	Model model;
	model.setTransform(Matrix::translation(10, 0, 0));
	model.skeleton().root().addJoint("shoulder", Matrix::translation(2, 0, 0));
	Part arm;
	arm.addJoint("pivot", Matrix::translation(1, 0, 0));
	arm.addJoint("elbow", Matrix::translation(3, 0, 0));
	model.skeleton().joint("shoulder").attach(arm, "pivot");
	model.skeleton().joint("shoulder").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));

	const auto rotation = Matrix::rotation(model.skeleton().joint("shoulder").rotation());
	const auto shoulder = Matrix::translation(10, 0, 0) * Matrix::translation(2, 0, 0) * rotation;
	const auto armMatrix = shoulder * Matrix::translation(-1, 0, 0);
	expectMatrix(model.globalMatrix(), Matrix::translation(10, 0, 0));
	expectMatrix(model.skeleton().root().globalMatrix(), model.globalMatrix());
	expectMatrix(model.skeleton().joint("shoulder").globalMatrix(), shoulder);
	expectMatrix(model.skeleton().joint("shoulder").child()->globalMatrix(), armMatrix);
	expectMatrix(model.skeleton().joint({"shoulder", "elbow"}).globalMatrix(), armMatrix * Matrix::translation(3, 0, 0));
	EXPECT_EQ(&model.globalMatrix(), &model.globalMatrix());
}

TEST(ModelCoreTest, CachedMatricesInvalidateAffectedBranchesAndAncestors)
{
	Model model;
	model.skeleton().root().addJoint("left", Matrix::translation(-2, 0, 0));
	model.skeleton().root().addJoint("right", Matrix::translation(2, 0, 0));
	model.skeleton().joint("left").attach(Part(volume()));
	model.skeleton().joint("right").attach(Part(volume()));
	(void)model.skeleton().joint("left").child()->globalMatrix();
	(void)model.skeleton().joint("right").child()->globalMatrix();

	model.skeleton().joint("left").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	const auto leftRotation = Matrix::rotation(model.skeleton().joint("left").rotation());
	expectMatrix(model.skeleton().joint("left").child()->globalMatrix(), Matrix::translation(-2, 0, 0) * leftRotation);
	expectMatrix(model.skeleton().joint("right").child()->globalMatrix(), Matrix::translation(2, 0, 0));

	model.setTransform(Matrix::translation(5, 0, 0));
	expectMatrix(model.skeleton().joint("left").child()->globalMatrix(), Matrix::translation(5, 0, 0) * Matrix::translation(-2, 0, 0) * leftRotation);
	expectMatrix(model.skeleton().joint("right").child()->globalMatrix(), Matrix::translation(7, 0, 0));
}

TEST(ModelCoreTest, AttachedModelCachesFollowParentAndNamedGrip)
{
	Model parent;
	parent.setTransform(Matrix::translation(10, 0, 0));
	parent.skeleton().root().addJoint("hand", Matrix::translation(4, 0, 0));
	Model sword;
	sword.setTransform(Matrix::translation(0, 2, 0));
	sword.skeleton().root().addJoint("grip", Matrix::translation(1, 0, 0));
	sword.skeleton().root().addJoint("tip", Matrix::translation(3, 0, 0));
	parent.skeleton().joint("hand").attach(sword, "grip");
	auto *attached = parent.skeleton().joint("hand").model();
	ASSERT_NE(attached, nullptr);
	const auto initial = Matrix::translation(10, 0, 0) * Matrix::translation(4, 0, 0) *
		Matrix::translation(-1, 0, 0) * Matrix::translation(0, 2, 0);
	expectMatrix(attached->globalMatrix(), initial);
	expectMatrix(attached->skeleton().joint("tip").globalMatrix(), initial * Matrix::translation(3, 0, 0));

	attached->skeleton().joint("grip").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	expectMatrix(attached->globalMatrix(), initial);
	attached->skeleton().joint("grip").setTransform(Matrix::translation(2, 0, 0));
	const auto movedGrip = Matrix::translation(10, 0, 0) * Matrix::translation(4, 0, 0) *
		Matrix::translation(-2, 0, 0) * Matrix::translation(0, 2, 0);
	expectMatrix(attached->globalMatrix(), movedGrip);
	parent.skeleton().joint("hand").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	const auto rotated = Matrix::translation(10, 0, 0) * Matrix::translation(4, 0, 0) *
		Matrix::rotation(parent.skeleton().joint("hand").rotation()) *
		Matrix::translation(-2, 0, 0) * Matrix::translation(0, 2, 0);
	expectMatrix(attached->globalMatrix(), rotated);
}

TEST(ModelCoreTest, CopiedAndMovedModelsHaveIndependentCachedMatrices)
{
	Model source;
	source.setTransform(Matrix::translation(3, 0, 0));
	source.skeleton().root().addJoint("arm", Matrix::translation(2, 0, 0));
	Part part;
	part.addJoint("elbow", Matrix::translation(1, 0, 0));
	source.skeleton().joint("arm").attach(part);
	(void)source.skeleton().joint({"arm", "elbow"}).globalMatrix();
	Model copy(source);
	Model moved(std::move(copy));
	expectMatrix(moved.skeleton().joint({"arm", "elbow"}).globalMatrix(), Matrix::translation(6, 0, 0));

	source.skeleton().joint("arm").setTransform(Matrix::translation(5, 0, 0));
	expectMatrix(source.skeleton().joint({"arm", "elbow"}).globalMatrix(), Matrix::translation(9, 0, 0));
	expectMatrix(moved.skeleton().joint({"arm", "elbow"}).globalMatrix(), Matrix::translation(6, 0, 0));

	Model assigned;
	assigned = moved;
	expectMatrix(assigned.skeleton().joint({"arm", "elbow"}).globalMatrix(), Matrix::translation(6, 0, 0));
	assigned = std::move(moved);
	expectMatrix(assigned.skeleton().joint({"arm", "elbow"}).globalMatrix(), Matrix::translation(6, 0, 0));
}

TEST(ModelCoreTest, ReplacingAttachedModelAndSkeletonRebindsCachedMatrix)
{
	Model parent;
	parent.skeleton().root().addJoint("mount", Matrix::translation(8, 0, 0));
	parent.skeleton().joint("mount").attach(Model{});
	auto *attached = parent.skeleton().joint("mount").model();
	ASSERT_NE(attached, nullptr);
	expectMatrix(attached->globalMatrix(), Matrix::translation(8, 0, 0));

	Model replacement;
	replacement.setTransform(Matrix::translation(2, 0, 0));
	replacement.skeleton().root().addJoint("tip", Matrix::translation(3, 0, 0));
	*attached = std::move(replacement);
	expectMatrix(attached->globalMatrix(), Matrix::translation(10, 0, 0));
	expectMatrix(attached->skeleton().joint("tip").globalMatrix(), Matrix::translation(13, 0, 0));

	Skeleton newSkeleton;
	newSkeleton.root().addJoint("tip", Matrix::translation(5, 0, 0));
	attached->skeleton() = std::move(newSkeleton);
	expectMatrix(attached->skeleton().joint("tip").globalMatrix(), Matrix::translation(15, 0, 0));
}
