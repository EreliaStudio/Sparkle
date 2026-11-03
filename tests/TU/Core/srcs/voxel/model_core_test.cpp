#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

#include "exception.hpp"
#include "model.hpp"
#include "voxel/volume.hpp"

namespace
{
	using Model = spk::Model;
	using Skeleton = spk::Skeleton;
	using Part = Skeleton::Part;
	using Matrix = spk::Matrix4x4;

	[[nodiscard]] const spk::Voxel::Volume &volume()
	{
		static const spk::Voxel::Volume value = [] {
			spk::Voxel::Volume::Builder builder({1, 1, 1});
			return builder.build();
		}();
		return value;
	}

	[[nodiscard]] Matrix referenceQuarterTurn(int turns, float x, float y, float z = 0.0f)
	{
		Matrix result = Matrix::identity();
		const int orientation = turns % 4;
		const std::array<float, 4> cosine{1, 0, -1, 0};
		const std::array<float, 4> sine{0, 1, 0, -1};
		result[0][0] = cosine[orientation];
		result[0][1] = sine[orientation];
		result[1][0] = -sine[orientation];
		result[1][1] = cosine[orientation];
		result[3][0] = x;
		result[3][1] = y;
		result[3][2] = z;
		return result;
	}

	[[nodiscard]] std::array<float, 2> referenceStep(int turns, float distance)
	{
		const std::array<float, 4> x{distance, 0, -distance, 0};
		const std::array<float, 4> y{0, distance, 0, -distance};
		return {x[turns % 4], y[turns % 4]};
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
	EXPECT_EQ(model.skeleton().root().body(), nullptr);
	model.skeleton().root().addJoint("hand");
	EXPECT_EQ(model.skeleton().joint("hand").child(), nullptr);
	EXPECT_EQ(model.version().topology, 1u);
}

TEST(ModelCoreTest, PartsReferenceImmutableVoxelVolumesWithoutOwnership)
{
	Model model;
	const auto original = volume();
	model.skeleton().root().setBody(&original);
	ASSERT_NE(model.skeleton().root().body(), nullptr);
	EXPECT_EQ(static_cast<const spk::Voxel::Volume *>(model.skeleton().root().body())->dimensions(), original.dimensions());
	EXPECT_EQ(model.version().geometry, 1u);
	model.skeleton().root().setBody(nullptr);
	EXPECT_EQ(model.skeleton().root().body(), nullptr);
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
	skeleton.root().joint("arm").attach(Part(&volume()));
	Model first(std::move(skeleton));
	Model second(first);
	first.skeleton().joint("arm").setRotation(spk::Quaternion::fromEuler({0, 0, 45}));
	EXPECT_NE(first.skeleton().joint("arm").rotation(), second.skeleton().joint("arm").rotation());
	ASSERT_NE(second.skeleton().joint("arm").child()->body(), nullptr);
	const auto firstCell = static_cast<const spk::Voxel::Volume *>(first.skeleton().joint("arm").child()->body())->atRaw({0, 0, 0});
	const auto secondCell = static_cast<const spk::Voxel::Volume *>(second.skeleton().joint("arm").child()->body())->atRaw({0, 0, 0});
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
	replacement.attach(Part(&volume()));
	model.skeleton().joint("arm") = std::move(replacement);
	EXPECT_GT(model.version().topology, before.topology);
	ASSERT_NE(model.skeleton().joint("arm").child(), nullptr);
	EXPECT_NE(model.skeleton().joint("arm").child()->body(), nullptr);
}

TEST(ModelCoreTest, JointCanAttachIndependentModelWithNamedGrip)
{
	Model character;
	character.skeleton().root().addJoint("hand");
	Model sword;
	sword.skeleton().root().setBody(&volume());
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
	EXPECT_NE(hand.model()->skeleton().root().body(), nullptr);
	hand.model()->setTransform(Matrix::translation(0, 2, 0));
	EXPECT_EQ(sword.transform()[3][1], 0.0f);
}

TEST(ModelCoreTest, JointSwitchesBetweenPartAndModelAndDetaches)
{
	Model character;
	character.skeleton().root().addJoint("hand");
	auto &hand = character.skeleton().joint("hand");
	hand.attach(Part(&volume()));
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
	attached->skeleton().root().setBody(&volume());
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
	expectMatrix(model.skeleton().joint(Skeleton::Path{"shoulder", "elbow"}).globalMatrix(), armMatrix * Matrix::translation(3, 0, 0));
	EXPECT_EQ(&model.globalMatrix(), &model.globalMatrix());
}

TEST(ModelCoreTest, CachedMatricesInvalidateAffectedBranchesAndAncestors)
{
	Model model;
	model.skeleton().root().addJoint("left", Matrix::translation(-2, 0, 0));
	model.skeleton().root().addJoint("right", Matrix::translation(2, 0, 0));
	model.skeleton().joint("left").attach(Part(&volume()));
	model.skeleton().joint("right").attach(Part(&volume()));
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
	const auto initial = Matrix::translation(10, 0, 0) * Matrix::translation(4, 0, 0) * Matrix::translation(-1, 0, 0) * Matrix::translation(0, 2, 0);
	expectMatrix(attached->globalMatrix(), initial);
	expectMatrix(attached->skeleton().joint("tip").globalMatrix(), initial * Matrix::translation(3, 0, 0));

	attached->skeleton().joint("grip").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	expectMatrix(attached->globalMatrix(), initial);
	attached->skeleton().joint("grip").setTransform(Matrix::translation(2, 0, 0));
	const auto movedGrip = Matrix::translation(10, 0, 0) * Matrix::translation(4, 0, 0) * Matrix::translation(-2, 0, 0) * Matrix::translation(0, 2, 0);
	expectMatrix(attached->globalMatrix(), movedGrip);
	parent.skeleton().joint("hand").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	const auto rotated = Matrix::translation(10, 0, 0) * Matrix::translation(4, 0, 0) * Matrix::rotation(parent.skeleton().joint("hand").rotation()) * Matrix::translation(-2, 0, 0) * Matrix::translation(0, 2, 0);
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
	(void)source.skeleton().joint(Skeleton::Path{"arm", "elbow"}).globalMatrix();
	Model copy(source);
	Model moved(std::move(copy));
	expectMatrix(moved.skeleton().joint(Skeleton::Path{"arm", "elbow"}).globalMatrix(), Matrix::translation(6, 0, 0));

	source.skeleton().joint("arm").setTransform(Matrix::translation(5, 0, 0));
	expectMatrix(source.skeleton().joint(Skeleton::Path{"arm", "elbow"}).globalMatrix(), Matrix::translation(9, 0, 0));
	expectMatrix(moved.skeleton().joint(Skeleton::Path{"arm", "elbow"}).globalMatrix(), Matrix::translation(6, 0, 0));

	Model assigned;
	assigned = moved;
	expectMatrix(assigned.skeleton().joint(Skeleton::Path{"arm", "elbow"}).globalMatrix(), Matrix::translation(6, 0, 0));
	assigned = std::move(moved);
	expectMatrix(assigned.skeleton().joint(Skeleton::Path{"arm", "elbow"}).globalMatrix(), Matrix::translation(6, 0, 0));
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

TEST(ModelCoreTest, PartAttachmentAnchorInvalidatesCachedPartMatrix)
{
	Model model;
	model.skeleton().root().addJoint("mount", Matrix::translation(5, 0, 0));
	Part part;
	part.addJoint("pivot", Matrix::translation(1, 0, 0));
	part.addJoint("tip", Matrix::translation(3, 0, 0));
	model.skeleton().joint("mount").attach(part, "pivot");
	auto *attached = model.skeleton().joint("mount").child();
	ASSERT_NE(attached, nullptr);
	expectMatrix(attached->globalMatrix(), Matrix::translation(4, 0, 0));
	expectMatrix(attached->joint("tip").globalMatrix(), Matrix::translation(7, 0, 0));

	attached->joint("pivot").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	expectMatrix(attached->globalMatrix(), Matrix::translation(4, 0, 0));
	attached->joint("pivot").setTransform(Matrix::translation(2, 0, 0));
	expectMatrix(attached->globalMatrix(), Matrix::translation(3, 0, 0));
	expectMatrix(attached->joint("tip").globalMatrix(), Matrix::translation(6, 0, 0));
}

TEST(ModelCoreTest, NestedAttachedModelsKeepTheirOwnHierarchyMatrix)
{
	Model character;
	character.setTransform(Matrix::translation(10, 0, 0));
	character.skeleton().root().addJoint("hand", Matrix::translation(2, 0, 0));
	Model sword;
	sword.skeleton().root().addJoint("tip", Matrix::translation(3, 0, 0));
	Model gem;
	gem.setTransform(Matrix::translation(0, 1, 0));
	sword.skeleton().joint("tip").attach(gem);
	character.skeleton().joint("hand").attach(sword);

	auto *attachedSword = character.skeleton().joint("hand").model();
	ASSERT_NE(attachedSword, nullptr);
	auto *attachedGem = attachedSword->skeleton().joint("tip").model();
	ASSERT_NE(attachedGem, nullptr);
	expectMatrix(attachedGem->globalMatrix(), Matrix::translation(15, 1, 0));

	const auto before = character.version();
	attachedSword->skeleton().joint("tip").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	const auto rotation = Matrix::rotation(attachedSword->skeleton().joint("tip").rotation());
	expectMatrix(attachedGem->globalMatrix(), Matrix::translation(15, 0, 0) * rotation * Matrix::translation(0, 1, 0));
	EXPECT_EQ(character.version(), before);

	character.setTransform(Matrix::translation(20, 0, 0));
	expectMatrix(attachedGem->globalMatrix(), Matrix::translation(25, 0, 0) * rotation * Matrix::translation(0, 1, 0));
}

TEST(ModelCoreTest, GoldenQuarterTurnMatricesIncludeChildAnchorOffset)
{
	Model character;
	character.setTransform(Matrix::translation(7, 2, 3));
	character.skeleton().root().addJoint("shoulder", Matrix::translation(2, 0, 0));
	Part arm;
	arm.addJoint("pivot", Matrix::translation(1, 0, 0));
	arm.addJoint("elbow", Matrix::translation(3, 0, 0));
	character.skeleton().joint("shoulder").attach(arm, "pivot");
	character.skeleton().joint("shoulder").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));

	const auto &shoulder = character.skeleton().joint("shoulder");
	ASSERT_NE(shoulder.child(), nullptr);
	expectMatrix(character.globalMatrix(), referenceQuarterTurn(0, 7, 2, 3));
	expectMatrix(character.skeleton().root().globalMatrix(), referenceQuarterTurn(0, 7, 2, 3));
	expectMatrix(shoulder.globalMatrix(), referenceQuarterTurn(1, 9, 2, 3));
	expectMatrix(shoulder.child()->globalMatrix(), referenceQuarterTurn(1, 9, 1, 3));
	expectMatrix(shoulder.child()->joint("elbow").globalMatrix(), referenceQuarterTurn(1, 9, 4, 3));

	shoulder.child()->joint("pivot").setRotation(spk::Quaternion::fromEuler({0, 0, 180}));
	expectMatrix(shoulder.child()->globalMatrix(), referenceQuarterTurn(1, 9, 1, 3));
}

TEST(ModelCoreTest, GoldenRotatedModelSocketInvertsRigidOrientation)
{
	Model character;
	character.skeleton().root().addJoint("hand", Matrix::translation(5, 3, 0));
	character.skeleton().joint("hand").setRotation(spk::Quaternion::fromEuler({0, 0, 180}));
	Model shield;
	shield.setTransform(Matrix::translation(0, 2, 0));
	const auto grip = Matrix::translation(1, 0, 0) * Matrix::rotation(0, 0, 90);
	shield.skeleton().root().addJoint("strap", grip);
	shield.skeleton().root().addJoint("edge", Matrix::translation(2, 0, 0));
	character.skeleton().joint("hand").attach(shield, "strap");
	const auto *equipped = character.skeleton().joint("hand").model();
	ASSERT_NE(equipped, nullptr);

	expectMatrix(character.skeleton().joint("hand").globalMatrix(), referenceQuarterTurn(2, 5, 3));
	expectMatrix(equipped->globalMatrix(), referenceQuarterTurn(1, 3, 2));
	expectMatrix(equipped->skeleton().root().globalMatrix(), referenceQuarterTurn(1, 3, 2));
	expectMatrix(equipped->skeleton().joint("edge").globalMatrix(), referenceQuarterTurn(1, 3, 4));

	equipped->skeleton().joint("strap").setRotation(spk::Quaternion::fromEuler({0, 0, 45}));
	expectMatrix(equipped->globalMatrix(), referenceQuarterTurn(1, 3, 2));
}

TEST(ModelCoreTest, GoldenNonUniformScaleFollowsCorrectMatrixOrder)
{
	Model model;
	model.setTransform(Matrix::translation(4, 0, 0) * Matrix::scale(2, 3, 1));
	model.skeleton().root().addJoint("shoulder", Matrix::translation(1, 1, 0));
	model.skeleton().joint("shoulder").attach(Part{});
	model.skeleton().joint("shoulder").setRotation(spk::Quaternion::fromEuler({0, 0, 90}));
	auto expected = Matrix::identity();
	expected[0][0] = 0;
	expected[0][1] = 3;
	expected[1][0] = -2;
	expected[1][1] = 0;
	expected[3][0] = 6;
	expected[3][1] = 3;
	expectMatrix(model.skeleton().joint("shoulder").globalMatrix(), expected);
	expectMatrix(model.skeleton().joint("shoulder").child()->globalMatrix(), expected);
}

TEST(ModelCoreTest, LongArticulatedChainMatchesIndependentPlanarReference)
{
	constexpr std::array<int, 8> turns{1, 3, 2, 1, 0, 3, 1, 2};
	Model model;
	Part chain;
	for (int i = static_cast<int>(turns.size()) - 1; i >= 0; --i)
	{
		Part parent;
		const auto name = "joint_" + std::to_string(i);
		parent.addJoint(name, Matrix::translation(2, 0, 0));
		parent.joint(name).attach(std::move(chain));
		chain = std::move(parent);
	}
	model.skeleton().root() = std::move(chain);
	model.setTransform(Matrix::translation(10, 20, 5));
	Part *current = &model.skeleton().root();
	for (std::size_t i = 0; i < turns.size(); ++i)
	{
		const auto name = "joint_" + std::to_string(i);
		current->joint(name).setRotation(spk::Quaternion::fromEuler({0, 0, static_cast<float>(turns[i] * 90)}));
		current = current->joint(name).child();
		ASSERT_NE(current, nullptr);
	}

	for (int phase = 0; phase < 3; ++phase)
	{
		float x = phase == 2 ? 30.0f : 10.0f;
		float y = 20;
		int orientation = 0;
		current = &model.skeleton().root();
		expectMatrix(current->globalMatrix(), referenceQuarterTurn(0, x, y, 5));
		for (std::size_t i = 0; i < turns.size(); ++i)
		{
			const auto step = referenceStep(orientation, 2);
			x += step[0];
			y += step[1];
			const int angle = phase >= 1 && i == 2 ? 3 : turns[i];
			orientation = (orientation + angle) % 4;
			const auto name = "joint_" + std::to_string(i);
			auto &joint = current->joint(name);
			expectMatrix(joint.globalMatrix(), referenceQuarterTurn(orientation, x, y, 5));
			current = joint.child();
			ASSERT_NE(current, nullptr);
			expectMatrix(current->globalMatrix(), referenceQuarterTurn(orientation, x, y, 5));
		}

		if (phase == 0)
		{
			model.skeleton().joint(Skeleton::Path{"joint_0", "joint_1", "joint_2"}).setRotation(spk::Quaternion::fromEuler({0, 0, 270}));
		}
		if (phase == 1)
		{
			model.setTransform(Matrix::translation(30, 20, 5));
		}
	}
}

TEST(ModelCoreTest, InvalidJointTransformDoesNotChangePreviouslyCachedMatrices)
{
	Model model;
	model.skeleton().root().addJoint("elbow", Matrix::translation(3, 2, 1));
	const auto before = model.skeleton().joint("elbow").globalMatrix();
	EXPECT_THROW(model.skeleton().joint("elbow").setTransform(Matrix::scale(2, 1, 1)), spk::Exception);
	EXPECT_THROW(model.skeleton().joint("elbow").setRotation({0, 0, 0, 0}), spk::Exception);
	expectMatrix(model.skeleton().joint("elbow").globalMatrix(), before);
}

TEST(ModelCoreTest, GoldenThreeDimensionalJointRotationsComposeInOrder)
{
	Model model;
	model.setTransform(Matrix::translation(1, 2, 3));
	const auto fixed = Matrix::translation(0, 1, 0) * Matrix::rotation(90, 0, 0);
	model.skeleton().root().addJoint("shoulder", fixed);
	Part arm;
	arm.addJoint("elbow", Matrix::translation(0, 0, 2));
	model.skeleton().joint("shoulder").attach(arm);
	model.skeleton().joint("shoulder").setRotation(spk::Quaternion::fromEuler({0, 90, 0}));

	Matrix expected = Matrix::identity();
	expected[0][0] = 0;
	expected[0][1] = 1;
	expected[1][1] = 0;
	expected[1][2] = 1;
	expected[2][2] = 0;
	expected[2][0] = 1;
	expected[3][0] = 1;
	expected[3][1] = 3;
	expected[3][2] = 3;
	expectMatrix(model.skeleton().joint("shoulder").globalMatrix(), expected);
	expectMatrix(model.skeleton().joint("shoulder").child()->globalMatrix(), expected);

	expected[3][0] = 3;
	expectMatrix(model.skeleton().joint(Skeleton::Path{"shoulder", "elbow"}).globalMatrix(), expected);
}

TEST(ModelCoreTest, StandaloneSkeletonReferencesAnyBodyWithoutOwningIt)
{
	struct GenericBody final : spk::Body
	{
	};
	GenericBody object;
	spk::Skeleton skeleton;
	skeleton.root().setBody(&object);
	skeleton.root().addJoint("socket", Matrix::translation(3, 0, 0));
	skeleton.joint("socket").attach(Part(&object));
	EXPECT_EQ(skeleton.root().body(), &object);
	ASSERT_NE(skeleton.joint("socket").child(), nullptr);
	EXPECT_EQ(skeleton.joint("socket").child()->body(), &object);
	expectMatrix(skeleton.joint("socket").globalMatrix(), Matrix::translation(3, 0, 0));

	Model model(std::move(skeleton));
	model.setTransform(Matrix::translation(5, 0, 0));
	EXPECT_EQ(model.skeleton().root().body(), &object);
	expectMatrix(model.skeleton().joint("socket").globalMatrix(), Matrix::translation(8, 0, 0));
	Model clone(model);
	EXPECT_EQ(clone.skeleton().root().body(), &object);
	EXPECT_EQ(clone.skeleton().joint("socket").child()->body(), &object);
}

TEST(ModelCoreTest, ModelDoesNotExtendBodyLifetime)
{
	struct TrackedBody final : spk::Body
	{
		bool *flag = nullptr;
		~TrackedBody() override
		{
			if (flag)
			{
				*flag = true;
			}
		}
	};
	bool destroyed = false;
	Skeleton skeleton;
	{
		TrackedBody body;
		body.flag = &destroyed;
		skeleton.root().setBody(&body);
		EXPECT_EQ(skeleton.root().body(), &body);
	}
	EXPECT_TRUE(destroyed);
	skeleton.root().setBody(nullptr);
	EXPECT_EQ(skeleton.root().body(), nullptr);
}
