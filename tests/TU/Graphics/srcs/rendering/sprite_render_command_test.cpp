#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <stdexcept>

#include "rendering/command/sprite_render_command.hpp"
#include "render_command_test_utils.hpp"
#include "ui/widget.hpp"

namespace test = render_command_test;

namespace
{
	[[nodiscard]] spk::Rect2D destination()
	{
		return spk::Rect2D{.anchor = {4, 6}, .size = {32, 24}};
	}

	[[nodiscard]] spk::SpriteSheet twoColorSheet()
	{
		constexpr std::array<std::uint8_t, 6> pixels{255, 0, 0, 0, 0, 255};
		const auto encoded = test::encodedPpm({2, 1}, pixels);
		return spk::SpriteSheet(encoded, {2, 1});
	}
}

TEST(SpriteRenderCommandTest, NullSpriteSheetIsRejected)
{
	EXPECT_THROW(
		(void)spk::SpriteRenderCommand(nullptr, spk::Vector2UInt{0, 0}, destination()),
		std::invalid_argument);
}

TEST(SpriteRenderCommandTest, FirstSpriteCoordinatesSelectFirstSection)
{
	auto sheet = twoColorSheet();
	test::Target target;
	target.clear();
	spk::SpriteRenderCommand(&sheet, {0, 0}, destination()).execute(target.context());
	EXPECT_EQ(test::pixel(target.capture(), {12, 12}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
}

TEST(SpriteRenderCommandTest, LastSpriteCoordinatesSelectLastSection)
{
	auto sheet = twoColorSheet();
	test::Target target;
	target.clear();
	spk::SpriteRenderCommand(&sheet, {1, 0}, destination()).execute(target.context());
	EXPECT_EQ(test::pixel(target.capture(), {12, 12}), (std::array<std::uint8_t, 4>{0, 0, 255, 255}));
}

TEST(SpriteRenderCommandTest, GeometryVariantsPreserveSelectedSprite)
{
	auto sheet = twoColorSheet();
	test::Target target;
	target.clear();
	spk::SpriteRenderCommand(&sheet, {0, 0}, {.anchor = {2, 2}, .size = {12, 18}}).execute(target.context());
	spk::SpriteRenderCommand(&sheet, {0, 0}, {.anchor = {30, 20}, .size = {28, 36}}).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {6, 6}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
	EXPECT_EQ(test::pixel(image, {40, 40}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
}

TEST(SpriteRenderCommandTest, DepthVariantsParticipateInDepthTesting)
{
	auto sheet = twoColorSheet();
	test::Target target;
	target.clear();
	spk::SpriteRenderCommand(&sheet, {0, 0}, destination(), 0.5f).execute(target.context());
	spk::SpriteRenderCommand(&sheet, {1, 0}, destination(), -0.5f).execute(target.context());
	EXPECT_EQ(test::pixel(target.capture(), {12, 12}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
}

TEST(SpriteRenderCommandTest, OutOfRangeCoordinatesPropagateSpriteSheetFailure)
{
	const auto *sheet = spk::Widget::defaultStyle->iconset.get();
	ASSERT_NE(sheet, nullptr);

	EXPECT_THROW(
		(void)spk::SpriteRenderCommand(sheet, {sheet->nbSprite().x, 0}, destination()),
		std::out_of_range);
	EXPECT_THROW(
		(void)spk::SpriteRenderCommand(sheet, {0, sheet->nbSprite().y}, destination()),
		std::out_of_range);
}
