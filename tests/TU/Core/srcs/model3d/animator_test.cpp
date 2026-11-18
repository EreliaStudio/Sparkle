#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "exception.hpp"
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
		Animation::Builder builder;
		builder.addStep({.time = 0.0f, .articulations = {{"torso", "shoulder", spk::Quaternion::identity()}}, .bodies = {{"arm", {1.0f, 1.0f, 1.0f}}}});
		builder.addStep({.time = 1.0f, .articulations = {{"torso", "shoulder", spk::Quaternion::fromEuler({0, 0, 90})}}, .bodies = {{"arm", {1.0f, 2.0f, 1.0f}}}});
		return builder.build();
	}

	[[nodiscard]] Animation legAnimation()
	{
		Animation::Builder builder;
		builder.addStep({.time = 0.0f, .articulations = {{"torso", "hip", {}}}});
		builder.addStep({.time = 1.0f, .articulations = {{"torso", "hip", spk::Quaternion::fromEuler({20, 0, 0})}}});
		return builder.build();
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

TEST(Model3DAnimationTest, RejectsInvalidDurationAndStepsWithoutChangingTheClip)
{
	Animation::Builder withoutDuration;
	EXPECT_THROW(static_cast<void>(withoutDuration.build()), std::invalid_argument);
	withoutDuration.addStep({.time = 0.0f});
	EXPECT_THROW(static_cast<void>(withoutDuration.build()), std::invalid_argument);
	withoutDuration.addStep({.time = 2.5f});
	EXPECT_FLOAT_EQ(withoutDuration.build().duration(), 2.5f);

	Animation::Builder builder;
	EXPECT_THROW(builder.addStep({.time = -0.1f}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = std::numeric_limits<float>::infinity()}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = std::numeric_limits<float>::quiet_NaN()}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .articulations = {{"", "shoulder", {}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .articulations = {{"torso", "", {}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .articulations = {{"torso", "shoulder", {0, 0, 0, 0}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .bodies = {{"torso", {0, 1, 1}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .bodies = {{"", {1, 1, 1}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .articulations = {{"torso", "shoulder", {}}, {"torso", "shoulder", {}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .bodies = {{"torso", {1, 1, 1}}, {"torso", {1, 1, 1}}}}), std::invalid_argument);

	builder.addStep({.time = 0.0f, .articulations = {{"torso", "shoulder", {0, 0, 0, 2}}}});

	EXPECT_THROW(builder.addStep({.time = 0.0f}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.8f, .bodies = {{"torso", {-1, 1, 1}}}}), std::invalid_argument);

	builder.addStep({.time = 0.8f, .articulations = {{"torso", "shoulder", {}}}});

	EXPECT_THROW(builder.addStep({.time = 0.4f}), std::invalid_argument);
	const Animation result = builder.build();
	EXPECT_FLOAT_EQ(result.duration(), 0.8f);
	ASSERT_EQ(result.steps().size(), 2u);
	EXPECT_NEAR(result.steps().front().articulations.front().rotation.w, 1.0f, 0.00001f);
	EXPECT_THROW(builder.addStep({.time = 0.9f}), std::logic_error);
	EXPECT_THROW(static_cast<void>(builder.build()), std::logic_error);
}

TEST(Model3DAnimatorTest, InterpolatesArticulationsAndBodyScaleIndependently)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	const auto handBefore = object.element("hand").globalTransform();
	const Animator::Handle handle = animator.play(armAnimation());
	EXPECT_TRUE(animator.playing(handle));
	animator.update(0.5f);
	const auto &shoulder = object.attachment("torso", "shoulder");
	EXPECT_EQ(shoulder.scale(), (spk::Vector3{1, 1, 1}));
	EXPECT_NEAR(shoulder.rotationMatrix()[0][0], std::sqrt(0.5f), 0.0001f);
	EXPECT_EQ(object.element("arm").scale(), (spk::Vector3{1, 1.5f, 1}));
	EXPECT_NE(object.element("hand").globalTransform(), handBefore);
	animator.update(0.5f);
	EXPECT_FALSE(animator.playing(handle));
	EXPECT_EQ(animator.activeCount(), 0u);
	EXPECT_EQ(shoulder.rotation(), spk::Quaternion::identity());
	EXPECT_EQ(object.element("arm").scale(), (spk::Vector3{1, 1, 1}));
	EXPECT_EQ(object.element("hand").globalTransform(), handBefore);
}

TEST(Model3DAnimatorTest, ElementScaleAnimatesRootAndPropagatesToChildren)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	Animation::Builder builder;
	builder.addStep({.time = 0.0f, .bodies = {{"torso", {1, 1, 1}}}});
	builder.addStep({.time = 1.0f, .bodies = {{"torso", {2, 3, 1}}}});
	const auto handBefore = object.element("hand").globalTransform();
	const auto handle = animator.play(builder.build());
	animator.update(0.5f);
	EXPECT_EQ(object.root().scale(), (spk::Vector3{1.5f, 2.0f, 1.0f}));
	EXPECT_NEAR(object.element("arm").globalTransform()[3][1], 2.0f, 0.0001f);
	EXPECT_NE(object.element("hand").globalTransform(), handBefore);
	EXPECT_TRUE(animator.stop(handle));
	EXPECT_EQ(object.root().scale(), (spk::Vector3{1, 1, 1}));
	EXPECT_EQ(object.element("hand").globalTransform(), handBefore);
}

TEST(Model3DAnimatorTest, ConcurrentDisjointMasksThrowOnConflictWithoutMutatingPose)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	const auto arm = animator.play(armAnimation());
	const auto leg = animator.play(legAnimation());
	ASSERT_EQ(animator.activeCount(), 2u);
	animator.update(0.25f);
	const auto before = object.attachment("torso", "shoulder").rotation();
	try
	{
		static_cast<void>(animator.play(armAnimation()));
		FAIL() << "Conflicting animation was accepted";
	} catch (const spk::Exception &exception)
	{
		const std::string message = exception.what();
		EXPECT_NE(message.find("torso/shoulder"), std::string::npos);
		EXPECT_NE(message.find(std::to_string(arm)), std::string::npos);
	}
	EXPECT_EQ(object.attachment("torso", "shoulder").rotation(), before);
	EXPECT_EQ(animator.activeCount(), 2u);
	EXPECT_TRUE(animator.stop(arm));
	const auto replacement = animator.play(armAnimation());
	EXPECT_TRUE(animator.playing(replacement));
	EXPECT_TRUE(animator.stop(leg));
}

TEST(Model3DAnimatorTest, EmptyMaskReservationsLastForTheEntireClip)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	Animation::Builder reserve;
	reserve.reserveArticulation("torso", "shoulder");
	reserve.reserveBody("arm");
	reserve.reserveArticulation("torso", "shoulder"); // Idempotent
	reserve.addStep({.time = 1.0f}); // A reservation-only animation still needs a final timestamp.
	const auto held = animator.play(reserve.build());
	EXPECT_TRUE(animator.playing(held));
	EXPECT_THROW(static_cast<void>(animator.play(armAnimation())), spk::Exception);
	animator.update(0.9f);
	EXPECT_THROW(static_cast<void>(animator.play(armAnimation())), spk::Exception);
	animator.update(0.1f);
	EXPECT_FALSE(animator.playing(held));
	const auto handle = animator.play(armAnimation());
	EXPECT_TRUE(animator.playing(handle));
}

TEST(Model3DAnimationTest, InvalidReservationsDoNotChangeAnimation)
{
	Animation::Builder builder;
	EXPECT_THROW(builder.reserveArticulation("torso", ""), std::invalid_argument);
	EXPECT_THROW(builder.reserveArticulation("", "shoulder"), std::invalid_argument);
	EXPECT_THROW(builder.reserveBody(""), std::invalid_argument);
	EXPECT_THROW(static_cast<void>(builder.build()), std::invalid_argument);
	builder.addStep({.time = 1.0f});
	const Animation clip = builder.build();
	EXPECT_EQ(clip.steps().size(), 1u);
}

TEST(Model3DAnimatorTest, InvalidTargetsAndDeltaCannotChangeRunningAnimations)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	Animation::Builder invalid;
	invalid.addStep({.time = 0.0f, .articulations = {{"torso", "missing", {}}}});
	invalid.addStep({.time = 1.0f});
	EXPECT_THROW(static_cast<void>(animator.play(invalid.build())), spk::Exception);
	EXPECT_EQ(animator.activeCount(), 0u);
	EXPECT_THROW(static_cast<void>(Animation::Builder().build()), std::invalid_argument);
	const auto handle = animator.play(armAnimation());
	animator.update(0.4f);
	const auto rotation = object.attachment("torso", "shoulder").rotation();
	EXPECT_THROW(animator.update(-0.1f), std::invalid_argument);
	EXPECT_THROW(animator.update(std::numeric_limits<float>::quiet_NaN()), std::invalid_argument);
	EXPECT_EQ(object.attachment("torso", "shoulder").rotation(), rotation);
	EXPECT_TRUE(animator.playing(handle));
}

TEST(Model3DAnimatorTest, LoopingUsesTimeAndRestoresCapturedPoseOnStop)
{
	ObjectHeadless object(characterBlueprint());
	object.attachment("torso", "shoulder").setRotation(spk::Vector3{0, 0, 12});
	object.attachment("torso", "shoulder").setScale({1, 1.25f, 1});
	object.element("arm").setScale({1, 1.25f, 1});
	const auto initial = object.attachment("torso", "shoulder").transform();
	Animator animator(object);
	const auto handle = animator.play(armAnimation(), true);
	animator.update(2.25f);
	const auto cycled = object.attachment("torso", "shoulder").rotationMatrix();
	ObjectHeadless other(characterBlueprint());
	Animator otherAnimator(other);
	const auto otherHandle = otherAnimator.play(armAnimation(), true);
	otherAnimator.update(0.25f);
	EXPECT_EQ(cycled, other.attachment("torso", "shoulder").rotationMatrix());
	EXPECT_TRUE(animator.stop(handle));
	EXPECT_TRUE(otherAnimator.playing(otherHandle));
	EXPECT_EQ(object.attachment("torso", "shoulder").transform(), initial);
	EXPECT_EQ(object.element("arm").scale(), (spk::Vector3{1, 1.25f, 1}));
	EXPECT_FALSE(animator.stop(handle));
}

TEST(Model3DAnimatorTest, AbsentEarlyStepUsesCapturedPoseAndSparseStepsInterpolate)
{
	ObjectHeadless object(characterBlueprint());
	object.attachment("torso", "shoulder").setRotation(spk::Vector3{0, 0, 10});
	const auto initial = object.attachment("torso", "shoulder").rotation();
	Animation::Builder clip;
	clip.addStep({.time = 0.2f, .bodies = {{"torso", {1, 1, 1}}}});
	clip.addStep({.time = 0.5f, .articulations = {{"torso", "shoulder", spk::Quaternion::fromEuler({0, 0, 90})}}});
	clip.addStep({.time = 0.75f, .bodies = {{"torso", {1, 2, 1}}}});
	clip.addStep({.time = 1.0f});
	Animator animator(object);
	const auto handle = animator.play(clip.build());
	EXPECT_EQ(object.attachment("torso", "shoulder").rotation(), initial);
	animator.update(0.25f);
	const auto expected = spk::Quaternion::slerp(initial, spk::Quaternion::fromEuler({0, 0, 90}), 0.5f);
	EXPECT_NEAR(std::fabs(object.attachment("torso", "shoulder").rotation().dot(expected)), 1.0f, 0.0001f);
	EXPECT_NEAR(object.root().scale().y, 1.0f + (0.25f - 0.2f) / (0.75f - 0.2f), 0.0001f);
	animator.update(0.5f);
	EXPECT_EQ(object.root().scale(), (spk::Vector3{1, 2, 1}));
	EXPECT_TRUE(animator.stop(handle));
	EXPECT_EQ(object.attachment("torso", "shoulder").rotation(), initial);
}

TEST(Model3DAnimatorTest, RotationAndScaleOnTheSameElementCanBeIndependentClips)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator(object);
	Animation::Builder rotate;
	rotate.addStep({.time = 0.0f, .articulations = {{"torso", "shoulder", {}}}});
	rotate.addStep({.time = 1.0f, .articulations = {{"torso", "shoulder", spk::Quaternion::fromEuler({0, 0, 90})}}});
	Animation::Builder scale;
	scale.addStep({.time = 0.0f, .bodies = {{"torso", {1, 1, 1}}}});
	scale.addStep({.time = 1.0f, .bodies = {{"torso", {2, 1, 1}}}});
	const auto rotationHandle = animator.play(rotate.build());
	const auto scaleHandle = animator.play(scale.build());
	animator.update(0.5f);
	EXPECT_EQ(object.root().scale(), (spk::Vector3{1.5f, 1, 1}));
	EXPECT_NEAR(object.attachment("torso", "shoulder").rotationMatrix()[0][0], std::sqrt(0.5f), 0.0001f);
	EXPECT_TRUE(animator.stop(rotationHandle));
	EXPECT_TRUE(animator.playing(scaleHandle));
}

TEST(Model3DAnimatorTest, CopiesOfBlueprintAreIndependentAndStopAllReleasesMasks)
{
	const auto clip = armAnimation();
	ObjectHeadless first(characterBlueprint());
	ObjectHeadless second(characterBlueprint());
	Animator a(first);
	Animator b(second);
	const auto firstHandle = a.play(clip);
	const auto secondHandle = b.play(clip);
	a.update(0.5f);
	b.update(0.25f);
	EXPECT_NE(first.attachment("torso", "shoulder").transform(), second.attachment("torso", "shoulder").transform());
	a.stopAll();
	EXPECT_EQ(a.activeCount(), 0u);
	EXPECT_EQ(first.attachment("torso", "shoulder").rotation(), spk::Quaternion::identity());
	EXPECT_NE(first.attachment("torso", "shoulder").transform(), second.attachment("torso", "shoulder").transform());
	EXPECT_FALSE(a.playing(firstHandle));
	EXPECT_TRUE(b.playing(secondHandle));
}
