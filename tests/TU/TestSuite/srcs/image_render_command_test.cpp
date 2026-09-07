#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include "rendering/command/image_render_command.hpp"
#include "rendering/command/scissor_render_command.hpp"
#include "render_command_test_utils.hpp"

namespace test = render_command_test;

namespace
{
	constexpr std::array<std::uint8_t, 16> pattern{
		255, 0, 0, 255, 0, 255, 0, 255,
		0, 0, 255, 255, 255, 255, 0, 255};
}

TEST(ImageRenderCommandTest, WholeTextureSectionFillsDestination)
{
	test::Texture texture({2, 2}, pattern);
	test::Target target;
	target.clear();
	spk::ImageRenderCommand(&texture, spk::Texture::Section::whole, {.anchor = {8, 8}, .size = {40, 40}}).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {14, 14}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
	EXPECT_EQ(test::pixel(image, {42, 42}), (std::array<std::uint8_t, 4>{255, 255, 0, 255}));
}

TEST(ImageRenderCommandTest, PartialTextureSectionUsesOnlyRequestedUVRegion)
{
	test::Texture texture({2, 2}, pattern);
	test::Target target;
	target.clear();
	spk::ImageRenderCommand(&texture, {{0, 0}, {0.5f, 0.5f}}, {.anchor = {8, 8}, .size = {40, 40}}).execute(target.context());
	EXPECT_EQ(test::pixel(target.capture(), {28, 28}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
}

TEST(ImageRenderCommandTest, EmptyDestinationProducesNoVisiblePixels)
{
	test::Texture texture({2, 2}, pattern);
	test::Target target;
	target.clear({0.25f, 0.5f, 0.75f, 1});
	const auto before = target.capture();
	spk::ImageRenderCommand(&texture, spk::Texture::Section::whole, {.anchor = {8, 8}, .size = {0, 32}}).execute(target.context());
	spk::ImageRenderCommand(&texture, spk::Texture::Section::whole, {.anchor = {8, 8}, .size = {32, 0}}).execute(target.context());
	EXPECT_EQ(target.capture().pixels, before.pixels);
}

TEST(ImageRenderCommandTest, ClippedDestinationOnlyAffectsVisiblePixels)
{
	const std::array<std::uint8_t, 4> whitePixel{255, 255, 255, 255};
	test::Texture texture({1, 1}, whitePixel);
	test::Target target;
	target.clear();
	spk::ScissorRenderCommand({.anchor = {16, 16}, .size = {16, 16}}).execute(target.context());
	spk::ImageRenderCommand(&texture, spk::Texture::Section::whole, {.anchor = {8, 8}, .size = {32, 32}}).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {20, 20}), (std::array<std::uint8_t, 4>{255, 255, 255, 255}));
	EXPECT_EQ(test::pixel(image, {12, 12}), (std::array<std::uint8_t, 4>{0, 0, 0, 0}));
}

TEST(ImageRenderCommandTest, OverlappingImagesRespectDepth)
{
	const std::array<std::uint8_t, 4> redPixel{255, 0, 0, 255};
	const std::array<std::uint8_t, 4> bluePixel{0, 0, 255, 255};
	test::Texture red({1, 1}, redPixel);
	test::Texture blue({1, 1}, bluePixel);
	test::Target target;
	target.clear();
	const spk::Rect2D overlap{.anchor = {8, 8}, .size = {32, 32}};
	spk::ImageRenderCommand(&red, spk::Texture::Section::whole, overlap, 0.5f).execute(target.context());
	spk::ImageRenderCommand(&blue, spk::Texture::Section::whole, overlap, -0.5f).execute(target.context());
	EXPECT_EQ(test::pixel(target.capture(), {20, 20}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
}

TEST(ImageRenderCommandTest, DifferentDestinationGeometriesMapTextureConsistently)
{
	test::Texture texture({2, 2}, pattern);
	test::Target target;
	target.clear();
	spk::ImageRenderCommand(&texture, spk::Texture::Section::whole, {.anchor = {2, 2}, .size = {20, 20}}).execute(target.context());
	spk::ImageRenderCommand(&texture, spk::Texture::Section::whole, {.anchor = {32, 8}, .size = {28, 48}}).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {6, 6}), test::pixel(image, {36, 16}));
	EXPECT_EQ(test::pixel(image, {18, 18}), test::pixel(image, {56, 48}));
}
