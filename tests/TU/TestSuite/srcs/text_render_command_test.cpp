#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "rendering/command/text_render_command.hpp"
#include "render_command_test_utils.hpp"
#include "ui/widget.hpp"

namespace test = render_command_test;

static_assert(!std::is_copy_constructible_v<spk::TextRenderCommand>);
static_assert(!std::is_copy_assignable_v<spk::TextRenderCommand>);

namespace
{
	using Horizontal = spk::Alignment::Horizontal;
	using Vertical = spk::Alignment::Vertical;

	struct Bounds
	{
		int left;
		int top;
		int right;
		int bottom;
	};

	std::optional<Bounds> visibleBounds(const sparkle_test::FramebufferImage &image)
	{
		Bounds result{static_cast<int>(image.size.x), static_cast<int>(image.size.y), -1, -1};
		for (unsigned int y = 0; y < image.size.y; ++y)
		{
			for (unsigned int x = 0; x < image.size.x; ++x)
			{
				if (test::pixel(image, {x, y})[3] == 0)
					continue;
				result.left = std::min(result.left, static_cast<int>(x));
				result.top = std::min(result.top, static_cast<int>(y));
				result.right = std::max(result.right, static_cast<int>(x));
				result.bottom = std::max(result.bottom, static_cast<int>(y));
			}
		}
		if (result.right < result.left)
			return std::nullopt;
		return result;
	}

	std::size_t countDominant(const sparkle_test::FramebufferImage &image, unsigned int channel, const spk::Rect2D *area = nullptr)
	{
		const spk::Rect2D bounds = area ? *area : spk::Rect2D{.anchor = {0, 0}, .size = image.size};
		std::size_t result = 0;
		for (unsigned int y = bounds.y; y < bounds.y + bounds.height; ++y)
			for (unsigned int x = bounds.x; x < bounds.x + bounds.width; ++x)
			{
				const auto value = test::pixel(image, {x, y});
				result += value[channel] > 32 && value[channel] > value[(channel + 1) % 3] * 2 && value[channel] > value[(channel + 2) % 3] * 2;
			}
		return result;
	}

	spk::TextRenderCommand::Anchor topLeft(spk::Vector2Int position = {16, 16})
	{
		return {position, {Horizontal::Left, Vertical::Top}};
	}

	std::filesystem::path fontPath()
	{
		return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path().parent_path() / "resources/fonts/arial.ttf";
	}
}

TEST(TextRenderCommandTest, NullFontIsRejected)
{
	EXPECT_THROW(
		(void)spk::TextRenderCommand(nullptr, spk::Font::Size{16, 0}, std::string_view("Sparkle"), {}, spk::Color{}),
		std::invalid_argument);
}

TEST(TextRenderCommandTest, Utf8TextRendersExpectedGlyphs)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	test::Target target({160, 96});
	target.clear();
	spk::TextRenderCommand(font, {36, 2}, "A\xc3\xa9", topLeft(), {1, 1, 1, 1}).execute(target.context());
	const auto bounds = visibleBounds(target.capture());
	ASSERT_TRUE(bounds.has_value());
	EXPECT_GT(bounds->right - bounds->left, 30);
}

TEST(TextRenderCommandTest, Utf32TextRendersExpectedGlyphs)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	test::Target target({160, 96});
	target.clear();
	spk::TextRenderCommand utf8(font, {36, 2}, "A\xc3\xa9", topLeft(), {1, 1, 1, 1});
	utf8.execute(target.context());
	const auto expected = target.capture();
	target.clear();
	spk::TextRenderCommand utf32(font, {36, 2}, U"A\u00e9", topLeft(), {1, 1, 1, 1});
	utf32.execute(target.context());
	EXPECT_EQ(target.capture().pixels, expected.pixels);
}

TEST(TextRenderCommandTest, AllHorizontalAndVerticalAlignmentsPlaceTextCorrectly)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	constexpr spk::Font::Size size{30, 1};
	const auto textSize = font->computeStringSize("Ag", size);
	const spk::Vector2Int anchor{96, 72};
	std::optional<Bounds> reference;
	for (const auto vertical : {Vertical::Top, Vertical::Center, Vertical::Bottom})
	{
		for (const auto horizontal : {Horizontal::Left, Horizontal::Center, Horizontal::Right})
		{
			test::Target target({192, 144});
			target.clear();
			spk::TextRenderCommand(font, size, "Ag", {anchor, {horizontal, vertical}}, {1, 1, 1, 1}).execute(target.context());
			const auto actual = visibleBounds(target.capture());
			ASSERT_TRUE(actual.has_value());
			if (!reference)
				reference = actual;
			const int expectedX = horizontal == Horizontal::Left ? 0 : horizontal == Horizontal::Center ? -static_cast<int>(textSize.x) / 2 : -static_cast<int>(textSize.x);
			const int expectedY = vertical == Vertical::Top ? 0 : vertical == Vertical::Center ? -static_cast<int>(textSize.y) / 2 : -static_cast<int>(textSize.y);
			EXPECT_EQ(actual->left - reference->left, expectedX);
			EXPECT_EQ(actual->top - reference->top, expectedY);
			EXPECT_EQ(actual->right - reference->right, expectedX);
			EXPECT_EQ(actual->bottom - reference->bottom, expectedY);
		}
	}
}

TEST(TextRenderCommandTest, GlyphColorIsApplied)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	test::Target target({128, 96});
	target.clear();
	spk::TextRenderCommand(font, {44, 0}, "A", topLeft(), {0, 1, 0, 1}).execute(target.context());
	const auto image = target.capture();
	EXPECT_GT(countDominant(image, 1), 0u);
	EXPECT_EQ(countDominant(image, 0), 0u);
}

TEST(TextRenderCommandTest, OutlineColorAndSizeAreApplied)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	test::Target target({128, 96});
	target.clear();
	spk::TextRenderCommand(font, {44, 6}, "A", topLeft(), {0, 1, 0, 1}, {1, 0, 0, 1}).execute(target.context());
	const auto image = target.capture();
	EXPECT_GT(countDominant(image, 0), 0u);
	EXPECT_GT(countDominant(image, 1), 0u);
}

TEST(TextRenderCommandTest, DepthParticipatesInOverlapOrdering)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	test::Target target({128, 96});
	target.clear();
	spk::TextRenderCommand(font, {44, 2}, "A", topLeft(), {1, 0, 0, 1}, {}, 0.5f).execute(target.context());
	const auto front = target.capture();
	spk::TextRenderCommand(font, {44, 2}, "A", topLeft(), {0, 0, 1, 1}, {}, -0.5f).execute(target.context());
	EXPECT_EQ(target.capture().pixels, front.pixels);
}

TEST(TextRenderCommandTest, EmptyTextProducesNoVisiblePixels)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	test::Target target({96, 64});
	target.clear({0.2f, 0.3f, 0.4f, 1});
	const auto expected = target.capture();
	spk::TextRenderCommand(font, {32}, std::string_view{}, topLeft(), {1, 1, 1, 1}).execute(target.context());
	spk::TextRenderCommand(font, {32}, spk::Font::Text{}, topLeft(), {1, 1, 1, 1}).execute(target.context());
	EXPECT_EQ(target.capture().pixels, expected.pixels);
}

TEST(TextRenderCommandTest, MissingGlyphUsesFontFallbackBehavior)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	test::Target target({128, 96});
	target.clear();
	spk::TextRenderCommand missing(font, {40, 2}, spk::Font::Text{U'\U0010ffff'}, topLeft(), {1, 1, 1, 1});
	missing.execute(target.context());
	const auto expected = target.capture();
	target.clear();
	spk::TextRenderCommand notdef(font, {40, 2}, spk::Font::Text{U'\0'}, topLeft(), {1, 1, 1, 1});
	notdef.execute(target.context());
	EXPECT_EQ(target.capture().pixels, expected.pixels);
}

TEST(TextRenderCommandTest, AtlasGrowthKeepsExistingTextCommandValid)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	constexpr spk::Font::Size size{47, 3};
	spk::TextRenderCommand command(font, size, "A", topLeft({24, 24}), {1, 1, 1, 1});
	test::Target target({128, 112});
	target.clear();
	command.execute(target.context());
	const auto expected = target.capture();
	auto &atlas = font->atlas(size);
	const auto originalSize = atlas.size();
	atlas.loadAllRenderableGlyphs();
	ASSERT_GT(atlas.size().x, originalSize.x);
	target.clear();
	command.execute(target.context());
	EXPECT_EQ(target.capture().pixels, expected.pixels);
}

TEST(TextRenderCommandTest, RepeatedExecutionProducesStablePixels)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	spk::TextRenderCommand command(font, {34, 2}, "Stable", topLeft(), {1, 1, 1, 1});
	test::Target target({180, 96});
	target.clear();
	command.execute(target.context());
	const auto expected = target.capture();
	target.clear();
	command.execute(target.context());
	EXPECT_EQ(target.capture().pixels, expected.pixels);
}

TEST(TextRenderCommandTest, MultipleCommandsSharingFontRemainIndependent)
{
	auto *font = spk::Widget::defaultStyle->font.get();
	test::Target target({192, 96});
	target.clear();
	spk::TextRenderCommand(font, {36, 2}, "Left", topLeft({8, 16}), {1, 0, 0, 1}).execute(target.context());
	spk::TextRenderCommand(font, {36, 2}, "Right", topLeft({104, 16}), {0, 1, 0, 1}).execute(target.context());
	const auto image = target.capture();
	const spk::Rect2D left{.anchor = {0, 0}, .size = {96, 96}};
	const spk::Rect2D right{.anchor = {96, 0}, .size = {96, 96}};
	EXPECT_GT(countDominant(image, 0, &left), 0u);
	EXPECT_EQ(countDominant(image, 1, &left), 0u);
	EXPECT_GT(countDominant(image, 1, &right), 0u);
	EXPECT_EQ(countDominant(image, 0, &right), 0u);
}

TEST(TextRenderCommandTest, FontLifetimeContractIsExercised)
{
	spk::Font source(fontPath());
	spk::Font owner;
	auto command = std::make_unique<spk::TextRenderCommand>(&source, spk::Font::Size{36, 2}, "A", topLeft(), spk::Color{1, 1, 1, 1});
	owner = std::move(source);
	test::Target target({96, 80});
	target.clear();
	command->execute(target.context());
	EXPECT_TRUE(visibleBounds(target.capture()).has_value());
	command.reset();
}
