#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include <chrono>

#include "core/context/update_context.hpp"
#include "ui/widget/animation_label.hpp"

namespace
{
	void expectWidgetImage(spk::Widget &widget, const std::filesystem::path &category, const std::string &name)
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset();

		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		spk::RenderSnapshot snapshot = builder.build();
		snapshot.execute(context.renderContext());

		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		const auto difference = sparkle_test::resultImagePath(category, name + "_difference");
		context.save(actual);

		ASSERT_TRUE(std::filesystem::exists(expected))
			<< "Missing golden image: " << expected << "\n"
			<< "The current render was saved to: " << actual;

		const sparkle_test::ImageComparisonResult result =
			sparkle_test::compareImages(actual, expected, difference);
		EXPECT_TRUE(result.matches)
			<< "Image mismatch for [" << category.string() << "/" << name << "]\n"
			<< "Different pixels: " << result.differentPixelCount << "\n"
			<< "Actual size: " << result.actualWidth << "x" << result.actualHeight << "\n"
			<< "Expected size: " << result.expectedWidth << "x" << result.expectedHeight << "\n"
			<< "Difference image: " << difference;
	}

	[[nodiscard]] const spk::Widget::Style &defaultStyle()
	{
		return spk::Widget::defaultStyle.get();
	}
}

TEST(AnimationLabelTest, DefaultConstructorExposesDefaultState)
{
	spk::AnimationLabel label("Animation");
	ASSERT_NE(label.spriteSheet(), nullptr);
	EXPECT_EQ(label.currentFrame(), 0u);
	EXPECT_EQ(label.rangeStart(), 0u);
	EXPECT_EQ(label.rangeEnd(), 99u);
	EXPECT_EQ(label.loopSpeed(), std::chrono::milliseconds(125));
	EXPECT_FLOAT_EQ(label.depth(), 0.0f);
}

TEST(AnimationLabelTest, SpriteSheetConstructorAndSetterExposeConfiguredSheet)
{
	const auto *sheet = defaultStyle().iconset.get();
	ASSERT_NE(sheet, nullptr);
	spk::AnimationLabel fromConstructor("Animation", sheet);
	EXPECT_EQ(fromConstructor.spriteSheet(), sheet);

	spk::AnimationLabel fromSetter("Animation");
	fromSetter.setSpriteSheet(sheet);
	EXPECT_EQ(fromSetter.spriteSheet(), sheet);
	EXPECT_EQ(fromSetter.currentFrame(), 0u);
}

TEST(AnimationLabelTest, ApplyStyleUsesStyleSpriteSheet)
{
	spk::AnimationLabel label("Animation");
	label.applyStyle(defaultStyle());
	EXPECT_EQ(label.spriteSheet(), defaultStyle().iconset.get());
}

TEST(AnimationLabelTest, LoopSpeedRangeAndDepthRoundTrip)
{
	spk::AnimationLabel label("Animation", defaultStyle().iconset.get());
	label.setAnimationRange(2, 5);
	label.setLoopSpeed(std::chrono::milliseconds(40));
	label.setDepth(3.5f);
	EXPECT_EQ(label.rangeStart(), 2u);
	EXPECT_EQ(label.rangeEnd(), 5u);
	EXPECT_EQ(label.currentFrame(), 2u);
	EXPECT_EQ(label.loopSpeed(), std::chrono::milliseconds(40));
	EXPECT_FLOAT_EQ(label.depth(), 3.5f);
}

TEST(AnimationLabelTest, InvalidAnimationRangeIsRejected)
{
	spk::AnimationLabel label("Animation", defaultStyle().iconset.get());
	EXPECT_THROW(label.setAnimationRange(4, 2), std::invalid_argument);
}

TEST(AnimationLabelTest, UpdateAdvancesAndWrapsConfiguredRange)
{
	spk::AnimationLabel label("Animation", defaultStyle().iconset.get());
	label.setAnimationRange(1, 2);
	label.setLoopSpeed(std::chrono::milliseconds(10));
	label.activate();
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext context{.time = {}, .deltaTime = std::chrono::milliseconds(10), .keyboard = keyboard, .mouse = mouse};
	label.updateState(context);
	EXPECT_EQ(label.currentFrame(), 2u);
	label.updateState(context);
	EXPECT_EQ(label.currentFrame(), 1u);
}

TEST(AnimationLabelTest, NullSheetAndOutOfSheetRangeAreRejected)
{
	spk::AnimationLabel label("Animation", defaultStyle().iconset.get());
	EXPECT_THROW(label.setSpriteSheet(nullptr), std::invalid_argument);
	const std::size_t count = static_cast<std::size_t>(defaultStyle().iconset->nbSprite().x) * defaultStyle().iconset->nbSprite().y;
	EXPECT_THROW(label.setAnimationRange(0, count), std::invalid_argument);
}

TEST(AnimationLabelTest, SheetReplacementRestoresFullRange)
{
	const auto *sheet = defaultStyle().iconset.get();
	spk::AnimationLabel label("Animation", sheet);
	label.setAnimationRange(2, 4);
	label.setLoopSpeed(std::chrono::milliseconds(17));
	label.setSpriteSheet(sheet);
	const std::size_t count = static_cast<std::size_t>(sheet->nbSprite().x) * sheet->nbSprite().y;
	EXPECT_EQ(label.currentFrame(), 0u);
	EXPECT_EQ(label.rangeStart(), 0u);
	EXPECT_EQ(label.rangeEnd(), count - 1);
}

TEST(AnimationLabelTest, PartialMultipleZeroAndNegativeDurationsFollowContract)
{
	spk::AnimationLabel label("Animation", defaultStyle().iconset.get());
	label.setAnimationRange(1, 3);
	label.setLoopSpeed(std::chrono::milliseconds(10));
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext partial{.time = {}, .deltaTime = std::chrono::milliseconds(9), .keyboard = keyboard, .mouse = mouse};
	label.updateState(partial);
	EXPECT_EQ(label.currentFrame(), 1u);
	partial.deltaTime = std::chrono::milliseconds(21);
	label.updateState(partial);
	EXPECT_EQ(label.currentFrame(), 1u);
	label.setLoopSpeed(spk::AnimationLabel::Duration::zero());
	label.updateState(partial);
	EXPECT_EQ(label.currentFrame(), 2u);
	label.setLoopSpeed(-std::chrono::milliseconds(1));
	label.updateState(partial);
	EXPECT_EQ(label.currentFrame(), 2u);
}

TEST(AnimationLabelTest, InactiveLabelDoesNotAdvance)
{
	spk::AnimationLabel label("Animation", defaultStyle().iconset.get());
	label.setAnimationRange(1, 2);
	label.setLoopSpeed(std::chrono::milliseconds(1));
	label.deactivate();
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext context{.time = {}, .deltaTime = std::chrono::milliseconds(20), .keyboard = keyboard, .mouse = mouse};
	label.updateState(context);
	EXPECT_EQ(label.currentFrame(), 1u);
}

TEST(AnimationLabelRenderTest, DISABLED_FirstFrame)
{
	spk::AnimationLabel label("Animation", defaultStyle().iconset.get());
	label.setGeometry({.anchor = {80, 70}, .size = {180, 180}});
	label.setAnimationRange(0, 3);
	label.activate();
	expectWidgetImage(label, "ui/widget/animation_label", "first_frame");
}

TEST(AnimationLabelRenderTest, DISABLED_AlternateFrame)
{
	spk::AnimationLabel label("Animation", defaultStyle().iconset.get());
	label.setGeometry({.anchor = {80, 70}, .size = {180, 180}});
	label.setAnimationRange(4, 6);
	label.setLoopSpeed(std::chrono::milliseconds(1));
	label.activate();
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext context{.time = {}, .deltaTime = std::chrono::milliseconds(1), .keyboard = keyboard, .mouse = mouse};
	label.updateState(context);
	expectWidgetImage(label, "ui/widget/animation_label", "alternate_frame");
}

TEST(AnimationLabelRenderTest, DISABLED_Resized)
{
	spk::AnimationLabel label("Animation", defaultStyle().iconset.get());
	label.setGeometry({.anchor = {40, 40}, .size = {320, 120}});
	label.activate();
	expectWidgetImage(label, "ui/widget/animation_label", "resized");
}
