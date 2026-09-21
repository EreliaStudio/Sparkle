#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "rendering/command/nine_slice_render_command.hpp"
#include "render_command_test_utils.hpp"
#include "ui/widget.hpp"

namespace test = render_command_test;

namespace
{
	[[nodiscard]] spk::Rect2D destination(spk::Vector2UInt size = {90, 90})
	{
		return spk::Rect2D{.anchor = {0, 0}, .size = size};
	}

	[[nodiscard]] spk::SpriteSheet coloredNineSlice()
	{
		constexpr std::array<std::uint8_t, 27> pixels{
			255, 0, 0, 0, 255, 0, 0, 0, 255,
			255, 255, 0, 255, 0, 255, 0, 255, 255,
			255, 128, 0, 255, 255, 255, 128, 128, 128};
		const auto encoded = test::encodedPpm({3, 3}, pixels);
		return spk::SpriteSheet(encoded, {3, 3});
	}

	[[nodiscard]] spk::SpriteSheet coordinateNineSlice()
	{
		std::vector<std::uint8_t> pixels;
		pixels.reserve(12 * 12 * 3);
		for (std::uint8_t y = 0; y < 12; ++y)
		{
			for (std::uint8_t x = 0; x < 12; ++x)
			{
				pixels.insert(
					pixels.end(),
					{static_cast<std::uint8_t>(x * 16), static_cast<std::uint8_t>(y * 16), 255});
			}
		}
		return spk::SpriteSheet(test::encodedPpm({12, 12}, pixels), {3, 3});
	}

	void expectOpaque(const sparkle_test::FramebufferImage &image, const spk::Rect2D &area)
	{
		for (unsigned int y = area.y; y < area.y + area.height; ++y)
			for (unsigned int x = area.x; x < area.x + area.width; ++x)
				EXPECT_EQ(test::pixel(image, {x, y})[3], 255) << "pixel (" << x << ", " << y << ")";
	}
}

TEST(NineSliceRenderCommandTest, NullSpriteSheetIsRejected)
{
	EXPECT_THROW((void)spk::NineSliceRenderCommand(nullptr, destination(), spk::Vector2UInt{10, 10}), std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, UnstretchedThreeByThreeSheetPreservesAllNineRegions)
{
	auto sheet = coloredNineSlice();
	test::Target target;
	target.clear();
	spk::NineSliceRenderCommand(&sheet, destination({30, 30}), {10, 10}).execute(target.context());
	const auto image = target.capture();
	constexpr std::array<std::array<std::uint8_t, 4>, 9> colors{{
		{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255},
		{255, 255, 0, 255}, {255, 0, 255, 255}, {0, 255, 255, 255},
		{255, 128, 0, 255}, {255, 255, 255, 255}, {128, 128, 128, 255}}};
	for (unsigned int y = 0; y < 3; ++y)
		for (unsigned int x = 0; x < 3; ++x)
			EXPECT_EQ(test::pixel(image, {x * 10 + 5, y * 10 + 5}), colors[y * 3 + x]);
}

TEST(NineSliceRenderCommandTest, DownscaledCornersUseDeterministicNearestTexels)
{
	auto sheet = coordinateNineSlice();
	test::Target target;
	target.clear();
	spk::NineSliceRenderCommand(&sheet, destination({12, 12}), {2, 2}).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {0, 0}), (std::array<std::uint8_t, 4>{16, 16, 255, 255}));
	EXPECT_EQ(test::pixel(image, {1, 0}), (std::array<std::uint8_t, 4>{48, 16, 255, 255}));
	EXPECT_EQ(test::pixel(image, {0, 1}), (std::array<std::uint8_t, 4>{16, 48, 255, 255}));
	EXPECT_EQ(test::pixel(image, {1, 1}), (std::array<std::uint8_t, 4>{48, 48, 255, 255}));
}

TEST(NineSliceRenderCommandTest, StretchedDestinationPreservesCornerSizes)
{
	auto sheet = coloredNineSlice();
	test::Target target;
	target.clear();
	spk::NineSliceRenderCommand(&sheet, destination({60, 50}), {8, 6}).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {7, 5}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
	EXPECT_EQ(test::pixel(image, {8, 5}), (std::array<std::uint8_t, 4>{0, 255, 0, 255}));
	EXPECT_EQ(test::pixel(image, {52, 5}), (std::array<std::uint8_t, 4>{0, 0, 255, 255}));
	EXPECT_EQ(test::pixel(image, {59, 49}), (std::array<std::uint8_t, 4>{128, 128, 128, 255}));
}

TEST(NineSliceRenderCommandTest, StretchedDestinationFillsHorizontalAndVerticalEdges)
{
	auto sheet = coloredNineSlice();
	test::Target target;
	target.clear();
	spk::NineSliceRenderCommand(&sheet, destination({60, 50}), {8, 6}).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {30, 3}), (std::array<std::uint8_t, 4>{0, 255, 0, 255}));
	EXPECT_EQ(test::pixel(image, {30, 47}), (std::array<std::uint8_t, 4>{255, 255, 255, 255}));
	EXPECT_EQ(test::pixel(image, {3, 25}), (std::array<std::uint8_t, 4>{255, 255, 0, 255}));
	EXPECT_EQ(test::pixel(image, {57, 25}), (std::array<std::uint8_t, 4>{0, 255, 255, 255}));
}

TEST(NineSliceRenderCommandTest, StretchedDestinationFillsCenterRegion)
{
	auto sheet = coloredNineSlice();
	test::Target target;
	target.clear();
	spk::NineSliceRenderCommand(&sheet, destination({60, 50}), {8, 6}).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {8, 6}), (std::array<std::uint8_t, 4>{255, 0, 255, 255}));
	EXPECT_EQ(test::pixel(image, {30, 25}), (std::array<std::uint8_t, 4>{255, 0, 255, 255}));
	EXPECT_EQ(test::pixel(image, {51, 43}), (std::array<std::uint8_t, 4>{255, 0, 255, 255}));
}

TEST(NineSliceRenderCommandTest, NonThreeByThreeSpriteSheetIsRejected)
{
	EXPECT_THROW((void)spk::NineSliceRenderCommand(spk::Widget::defaultStyle->iconset.get(), destination(), spk::Vector2UInt{10, 10}), std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, NonDivisibleSpriteCellsAreRejected)
{
	std::vector<std::uint8_t> pixels(10 * 9 * 3, 255);
	auto sheet = spk::SpriteSheet(test::encodedPpm({10, 9}, pixels), {3, 3});
	EXPECT_THROW((void)spk::NineSliceRenderCommand(&sheet, destination(), spk::Vector2UInt{10, 10}), std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, CornerWidthGreaterThanHalfDestinationIsRejected)
{
	EXPECT_THROW((void)spk::NineSliceRenderCommand(spk::Widget::defaultStyle->nineSlice.get(), destination({20, 40}), spk::Vector2UInt{11, 10}), std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, CornerHeightGreaterThanHalfDestinationIsRejected)
{
	EXPECT_THROW((void)spk::NineSliceRenderCommand(spk::Widget::defaultStyle->nineSlice.get(), destination({40, 20}), spk::Vector2UInt{10, 11}), std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, WideDestinationKeepsCornersAndFillsCenter)
{
	auto sheet = coloredNineSlice();
	test::Target target;
	target.clear();
	const spk::Rect2D area{.anchor = {2, 4}, .size = {60, 24}};
	spk::NineSliceRenderCommand(&sheet, area, {6, 6}).execute(target.context());
	expectOpaque(target.capture(), area);
}

TEST(NineSliceRenderCommandTest, TallDestinationKeepsCornersAndFillsCenter)
{
	auto sheet = coloredNineSlice();
	test::Target target;
	target.clear();
	const spk::Rect2D area{.anchor = {4, 2}, .size = {24, 60}};
	spk::NineSliceRenderCommand(&sheet, area, {6, 6}).execute(target.context());
	expectOpaque(target.capture(), area);
}
