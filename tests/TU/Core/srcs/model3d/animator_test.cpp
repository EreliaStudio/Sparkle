#include <cmath>
#include <limits>
#include <stdexcept>

#include <gtest/gtest.h>

#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "model3d/animation.hpp"
#include "model3d/animator.hpp"
#include "model3d/blueprint.hpp"
#include "model3d/object_headless.hpp"

namespace
{
	using spk::Model3D::Animation;
	using spk::Model3D::Animator;
	using spk::Model3D::Blueprint;
	using spk::Model3D::ObjectHeadless;

	[[nodiscard]] Blueprint characterBlueprint()
	{
		Blueprint::Builder builder;
		auto &torso = builder.setRoot<Blueprint::Element>("torso");
		torso.addAttachmentPoint("shoulder", {0, 1, 0});
		auto &arm = torso.attach<Blueprint::Element>("shoulder", "arm");
		arm.addAttachmentPoint("wrist", {0, -1, 0});
		arm.attach<Blueprint::Element>("wrist", "hand");
		torso.addAttachmentPoint("hip", {0, -1, 0});
		torso.attach<Blueprint::Element>("hip", "leg");
		return builder.build();
	}

	[[nodiscard]] Animation armAnimation()
	{
		Animation animation(1.0f);
		animation.addTrack({"torso", "shoulder"}, {{0.0f, {}, {1.0f, 1.0f, 1.0f}}, {1.0f, spk::Quaternion::fromEuler({0, 0, 90}), {1.0f, 2.0f, 1.0f}}});
		return animation;
	}

	[[nodiscard]] Animation legAnimation()
	{
		Animation animation(1.0f);
		animation.addTrack({"torso", "hip"}, {{0.0f, {}, {1.0f, 1.0f, 1.0f}}, {1.0f, spk::Quaternion::fromEuler({20, 0, 0}), {1.0f, 1.0f, 1.0f}}});
		return animation;
	}
}

TEST(VectorLerpTest, TwoThreeAndFourDimensionsSupportInterpolationAndExtrapolation)
{
	EXPECT_EQ(spk::Vector2::lerp({1, 2}, {5, 6}, 0.0f), (spk::Vector2{1, 2}));
	EXPECT_EQ(spk::Vector2::lerp({1, 2}, {5, 6}, 0.5f), (spk::Vector2{3, 4}));
	EXPECT_EQ(spk::Vector3::lerp({1, 2, 3}, {5, 6, 7}, 1.0f), (spk::Vector3{5, 6, 7}));
	EXPECT_EQ(spk::Vector3::lerp({1, 2, 3}, {5, 6, 7}, 0.5f), (spk::Vector3{3, 4, 5}));
	EXPECT_EQ(spk::Vector4::lerp({1, 2, 3, 4}, {5, 6, 7, 8}, 0.5f), (spk::Vector4{3, 4, 5, 6}));
	EXPECT_EQ(spk::Vector4::lerp({1, 2, 3, 4}, {5, 6, 7, 8}, 1.5f), (spk::Vector4{7, 8, 9, 10}));
}

TEST(Model3DAnimationTest, RejectsInvalidDurationAndKeyframesWithoutAddingTrack)
{
	EXPECT_THROW(static_cast<void>(Animation(0.0f)), std::invalid_argument);
	EXPECT_THROW(static_cast<void>(Animation(-1.0f)), std::invalid_argument);
	EXPECT_THROW(static_cast<void>(Animation(std::numeric_limits<float>::infinity())), std::invalid_argument);
	Animation animation(1.0f);
	EXPECT_THROW(animation.addTrack({"", "shoulder"}, {}), std::invalid_argument);
	EXPECT_THROW(animation.addTrack({"torso", "shoulder"}, {{0.5f}, {0.5f}}), std::invalid_argument);
	EXPECT_THROW(animation.addTrack({"torso", "shoulder"}, {{0.3f}, {0.2f}}), std::invalid_argument);
	EXPECT_THROW(animation.addTrack({"torso", "shoulder"}, {{1.1f}}), std::invalid_argument);
	EXPECT_THROW(animation.addTrack({"torso", "shoulder"}, {{-0.1f}}), std::invalid_argument);
	EXPECT_THROW(animation.addTrack({"torso", "shoulder"}, {{0.0f, {0, 0, 0, 0}}}), std::invalid_argument);
	EXPECT_THROW(animation.addTrack({"torso", "shoulder"}, {{0.0f, {}, {1, 0, 1}}}), std::invalid_argument);
	EXPECT_THROW(animation.addTrack({"torso", ""}, {{0.0f, spk::Quaternion::fromEuler({0, 0, 10})}}), std::invalid_argument);
	EXPECT_TRUE(animation.tracks().empty());
	animation.addTrack({"torso", "shoulder"}, {{0.0f, {0, 0, 0, 2}}});
	EXPECT_NEAR(animation.tracks().front().keyframes.front().rotation.w, 1.0f, 0.00001f);
	EXPECT_THROW(animation.addTrack({"torso", "shoulder"}, {}), std::invalid_argument);
}

TEST(Model3DAnimatorTest, InterpolatesRotationAndNonUniformScaleAndUpdatesDescendants)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	const auto before = object.element("hand").globalTransform();
	const auto result = animator.play(armAnimation());
	ASSERT_TRUE(result.started);
	EXPECT_TRUE(animator.playing(result.handle));
	animator.update(0.5f);
	const auto &shoulder = object.attachment("torso", "shoulder");
	EXPECT_NEAR(shoulder.scale().y, 1.5f, 0.0001f);
	EXPECT_NEAR(shoulder.rotationMatrix()[0][0], std::sqrt(0.5f), 0.0001f);
	EXPECT_NEAR(shoulder.transform()[1][1], 1.5f * std::sqrt(0.5f), 0.0001f);
	EXPECT_NE(object.element("hand").globalTransform(), before);
	animator.update(0.5f);
	EXPECT_FALSE(animator.playing(result.handle));
	EXPECT_EQ(animator.activeCount(), 0u);
	EXPECT_EQ(shoulder.rotation(), spk::Quaternion::identity());
	EXPECT_EQ(shoulder.scale(), (spk::Vector3{1, 1, 1}));
	EXPECT_EQ(object.element("hand").globalTransform(), before);
}

TEST(Model3DAnimatorTest, ElementScaleAnimatesRootAndPropagatesToAllChildren)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	Animation animation(1.0f);
	animation.addTrack({"torso", ""}, {{0.0f, {}, {1, 1, 1}}, {1.0f, {}, {2, 3, 1}}});
	const auto handBefore = object.element("hand").globalTransform();
	const auto result = animator.play(animation);
	ASSERT_TRUE(result.started);
	animator.update(0.5f);
	EXPECT_EQ(object.root().scale(), (spk::Vector3{1.5f, 2.0f, 1.0f}));
	EXPECT_NEAR(object.element("arm").globalTransform()[3][1], 2.0f, 0.0001f);
	EXPECT_NE(object.element("hand").globalTransform(), handBefore);
	EXPECT_TRUE(animator.stop(result.handle));
	EXPECT_EQ(object.root().scale(), (spk::Vector3{1, 1, 1}));
	EXPECT_EQ(object.element("hand").globalTransform(), handBefore);
}

TEST(Model3DAnimatorTest, ConcurrentDisjointMasksAndNonMutatingConflict)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	const auto arm = animator.play(armAnimation());
	const auto leg = animator.play(legAnimation());
	ASSERT_TRUE(arm.started);
	ASSERT_TRUE(leg.started);
	EXPECT_EQ(animator.activeCount(), 2u);
	animator.update(0.25f);
	const auto before = object.attachment("torso", "shoulder").rotation();
	const auto blocked = animator.play(armAnimation());
	EXPECT_FALSE(blocked.started);
	ASSERT_TRUE(blocked.conflict.has_value());
	EXPECT_EQ(blocked.conflict->owner, arm.handle);
	EXPECT_EQ(blocked.conflict->target, (Animation::Target{"torso", "shoulder"}));
	EXPECT_EQ(object.attachment("torso", "shoulder").rotation(), before);
	EXPECT_EQ(animator.activeCount(), 2u);
	EXPECT_TRUE(animator.stop(arm.handle));
	EXPECT_TRUE(animator.play(armAnimation()).started);
	EXPECT_TRUE(animator.stop(leg.handle));
}

TEST(Model3DAnimatorTest, ExplicitEmptyTrackReservesItsJointForEntireClip)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	Animation reserve(1.0f);
	reserve.addTrack({"torso", "shoulder"}, {});
	const auto held = animator.play(reserve);
	ASSERT_TRUE(held.started);
	EXPECT_FALSE(animator.play(armAnimation()).started);
	animator.update(0.9f);
	EXPECT_FALSE(animator.play(armAnimation()).started);
	animator.update(0.1f);
	EXPECT_TRUE(animator.play(armAnimation()).started);
}

TEST(Model3DAnimatorTest, FailedPlayAndInvalidDeltaDoNotChangePose)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	Animation invalidTarget(1.0f);
	invalidTarget.addTrack({"torso", "missing"}, {{0}});
	EXPECT_THROW(static_cast<void>(animator.play(invalidTarget)), spk::Exception);
	EXPECT_EQ(animator.activeCount(), 0u);
	const auto result = animator.play(armAnimation());
	ASSERT_TRUE(result.started);
	animator.update(0.4f);
	const auto rotation = object.attachment("torso", "shoulder").rotation();
	EXPECT_THROW(animator.update(-0.1f), std::invalid_argument);
	EXPECT_THROW(animator.update(std::numeric_limits<float>::quiet_NaN()), std::invalid_argument);
	EXPECT_EQ(object.attachment("torso", "shoulder").rotation(), rotation);
}

TEST(Model3DAnimatorTest, LoopingIsTimeBasedAndStopRestoresCapturedPose)
{
	ObjectHeadless object(characterBlueprint());
	object.attachment("torso", "shoulder").setRotation(spk::Vector3{0, 0, 12});
	object.attachment("torso", "shoulder").setScale({1, 1.25f, 1});
	const auto initial = object.attachment("torso", "shoulder").transform();
	Animator animator(object);
	const auto result = animator.play(armAnimation(), true);
	ASSERT_TRUE(result.started);
	animator.update(2.25f);
	const auto cycled = object.attachment("torso", "shoulder").transform();
	ObjectHeadless other(characterBlueprint());
	Animator otherAnimator(other);
	const auto otherClip = otherAnimator.play(armAnimation(), true);
	ASSERT_TRUE(otherClip.started);
	otherAnimator.update(0.25f);
	EXPECT_EQ(cycled, other.attachment("torso", "shoulder").transform());
	EXPECT_TRUE(animator.stop(result.handle));
	EXPECT_EQ(object.attachment("torso", "shoulder").transform(), initial);
	EXPECT_FALSE(animator.stop(result.handle));
}

TEST(Model3DAnimatorTest, CopiesOfBlueprintAreIndependentAndStopAllReleasesMasks)
{
	const auto clip = armAnimation();
	ObjectHeadless first(characterBlueprint());
	ObjectHeadless second(characterBlueprint());
	Animator a(first);
	Animator b(second);
	EXPECT_TRUE(a.play(clip).started);
	EXPECT_TRUE(b.play(clip).started);
	a.update(0.5f);
	b.update(0.25f);
	EXPECT_NE(first.attachment("torso", "shoulder").transform(), second.attachment("torso", "shoulder").transform());
	a.stopAll();
	EXPECT_EQ(a.activeCount(), 0u);
	EXPECT_EQ(first.attachment("torso", "shoulder").rotation(), spk::Quaternion::identity());
	EXPECT_NE(first.attachment("torso", "shoulder").transform(), second.attachment("torso", "shoulder").transform());
}
