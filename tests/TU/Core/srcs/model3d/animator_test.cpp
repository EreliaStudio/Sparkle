#include <array>
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
		builder.addStep({.time = 0.0f, .articulations = {{"torso", "shoulder", spk::Quaternion::identity()}}, .scalings = {{"arm", {1.0f, 1.0f, 1.0f}}}});
		builder.addStep({.time = 1.0f, .articulations = {{"torso", "shoulder", spk::Quaternion::fromEuler({0, 0, 90})}}, .scalings = {{"arm", {1.0f, 2.0f, 1.0f}}}});
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
	EXPECT_THROW(builder.addStep({.time = 0.0f, .scalings = {{"torso", {0, 1, 1}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .scalings = {{"", {1, 1, 1}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .articulations = {{"torso", "shoulder", {}}, {"torso", "shoulder", {}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .scalings = {{"torso", {1, 1, 1}}, {"torso", {1, 1, 1}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .translations = {{"torso", {std::numeric_limits<float>::infinity(), 0, 0}}}, .scalings = {{"torso", {1, 1, 1}}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .translations = {{"torso", {}, static_cast<Animation::Interpolation>(99)}}, .scalings = {{"torso", {1, 1, 1}, static_cast<Animation::Interpolation>(99)}}}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.0f, .articulations = {{"torso", "shoulder", {}, static_cast<Animation::Interpolation>(99)}}}), std::invalid_argument);

	builder.addStep({.time = 0.0f, .articulations = {{"torso", "shoulder", {0, 0, 0, 2}}}});

	EXPECT_THROW(builder.addStep({.time = 0.0f}), std::invalid_argument);
	EXPECT_THROW(builder.addStep({.time = 0.8f, .scalings = {{"torso", {-1, 1, 1}}}}), std::invalid_argument);

	builder.addStep({.time = 0.8f, .articulations = {{"torso", "shoulder", {}}}});

	EXPECT_THROW(builder.addStep({.time = 0.4f}), std::invalid_argument);
	const Animation result = builder.build();
	EXPECT_FLOAT_EQ(result.duration(), 0.8f);
	ASSERT_EQ(result.steps().size(), 2u);
	EXPECT_NEAR(result.steps().front().articulations.front().rotation.w, 1.0f, 0.00001f);
	EXPECT_THROW(builder.addStep({.time = 0.9f}), std::logic_error);
	EXPECT_THROW(static_cast<void>(builder.build()), std::logic_error);
}


TEST(Model3DAnimatorTest, NamedAnimationsRestartAndKeepPoseOnStop)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator = Animator::Builder().addAnimation("arm", armAnimation()).build();
	animator.bind(object);
	animator.play("arm");
	animator.update(0.5f);
	EXPECT_EQ(object.element("arm").scale(), (spk::Vector3{1, 1.5f, 1}));
	animator.play("arm");
	EXPECT_TRUE(animator.playing("arm"));
	animator.update(0.25f);
	EXPECT_EQ(object.element("arm").scale(), (spk::Vector3{1, 1.25f, 1}));
	EXPECT_TRUE(animator.stop("arm"));
	EXPECT_FALSE(animator.playing("arm"));
	EXPECT_EQ(object.element("arm").scale(), (spk::Vector3{1, 1.25f, 1}));
}

TEST(Model3DAnimatorTest, DefaultIsSuspendedAndExitRunsOnlyAfterNaturalCompletion)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator = Animator::Builder()
		.addAnimation("idle", legAnimation())
		.addAnimation("attack", armAnimation())
		.addAnimation("recovery", armAnimation())
		.setDefaultAnimation("idle")
		.setExitAnimation("attack", "recovery")
		.build();
	animator.bind(object);
	EXPECT_TRUE(animator.playing("idle"));
	animator.play("attack");
	EXPECT_FALSE(animator.playing("idle"));
	animator.update(1.0f);
	EXPECT_FALSE(animator.playing("attack"));
	EXPECT_TRUE(animator.playing("recovery"));
	animator.update(1.0f);
	EXPECT_TRUE(animator.playing("idle"));
	animator.play("attack");
	EXPECT_TRUE(animator.stop("attack"));
	EXPECT_FALSE(animator.playing("recovery"));
	EXPECT_TRUE(animator.playing("idle"));
}

TEST(Model3DAnimatorTest, DisjointAnimationsAndConflict)
{
	ObjectHeadless object(characterBlueprint());
	Animator animator = Animator::Builder()
		.addAnimation("arm", armAnimation())
		.addAnimation("leg", legAnimation())
		.addAnimation("arm2", armAnimation())
		.build();
	animator.bind(object);
	animator.play("arm");
	animator.play("leg");
	EXPECT_EQ(animator.activeCount(), 2u);
	EXPECT_THROW(animator.play("arm2"), spk::Exception);
	EXPECT_EQ(animator.activeCount(), 2u);
	animator.stopAll();
	EXPECT_EQ(animator.activeCount(), 0u);
}

TEST(Model3DAnimatorTest, BuilderValidatesNamesAndReferences)
{
	Animator::Builder builder;
	builder.addAnimation("idle", armAnimation());
	EXPECT_THROW(builder.addAnimation("idle", armAnimation()), std::invalid_argument);
	builder.setExitAnimation("idle", "missing");
	EXPECT_THROW(static_cast<void>(builder.build()), std::invalid_argument);
	Animator::Builder invalidDefault;
	invalidDefault.setDefaultAnimation("missing");
	EXPECT_THROW(static_cast<void>(invalidDefault.build()), std::invalid_argument);
}

TEST(Model3DAnimatorTest, AnimationCopiesShareImmutableData)
{
	const Animation first = armAnimation();
	const Animation second = first;
	EXPECT_EQ(&first.steps(), &second.steps());
	EXPECT_EQ(first.duration(), second.duration());
}
