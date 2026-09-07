#include <gtest/gtest.h>

#include <stdexcept>

#include "rendering/command/nine_slice_render_command.hpp"
#include "ui/widget.hpp"

namespace
{
	[[nodiscard]] spk::Rect2D destination(spk::Vector2UInt size = {90, 90})
	{
		return spk::Rect2D{.anchor = {0, 0}, .size = size};
	}
}

TEST(NineSliceRenderCommandTest, NullSpriteSheetIsRejected)
{
	EXPECT_THROW(
		(void)spk::NineSliceRenderCommand(nullptr, destination(), spk::Vector2UInt{10, 10}),
		std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, DISABLED_UnstretchedThreeByThreeSheetPreservesAllNineRegions)
{
	GTEST_SKIP() << "Requires a concrete 3x3 SpriteSheet and deterministic Texture/offscreen readback APIs not included in section 10.";
	// Intended assertion: destination matching the source nine-slice layout reproduces all nine cells without distortion.
}

TEST(NineSliceRenderCommandTest, DISABLED_StretchedDestinationPreservesCornerSizes)
{
	GTEST_SKIP() << "Requires a concrete 3x3 SpriteSheet and deterministic offscreen readback APIs not included in section 10.";
	// Intended assertion: corners retain cornerSize while the destination grows.
}

TEST(NineSliceRenderCommandTest, DISABLED_StretchedDestinationFillsHorizontalAndVerticalEdges)
{
	GTEST_SKIP() << "Requires a concrete 3x3 SpriteSheet and deterministic offscreen readback APIs not included in section 10.";
	// Intended assertion: top/bottom edges stretch horizontally and left/right edges stretch vertically between preserved corners.
}

TEST(NineSliceRenderCommandTest, DISABLED_StretchedDestinationFillsCenterRegion)
{
	GTEST_SKIP() << "Requires a concrete 3x3 SpriteSheet and deterministic offscreen readback APIs not included in section 10.";
	// Intended assertion: the center sprite fills the remaining center rectangle.
}

TEST(NineSliceRenderCommandTest, NonThreeByThreeSpriteSheetIsRejected)
{
	EXPECT_THROW(
		(void)spk::NineSliceRenderCommand(
			spk::Widget::defaultStyle->iconset.get(),
			destination(),
			spk::Vector2UInt{10, 10}),
		std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, CornerWidthGreaterThanHalfDestinationIsRejected)
{
	EXPECT_THROW(
		(void)spk::NineSliceRenderCommand(
			spk::Widget::defaultStyle->nineSlice.get(),
			destination({20, 40}),
			spk::Vector2UInt{11, 10}),
		std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, CornerHeightGreaterThanHalfDestinationIsRejected)
{
	EXPECT_THROW(
		(void)spk::NineSliceRenderCommand(
			spk::Widget::defaultStyle->nineSlice.get(),
			destination({40, 20}),
			spk::Vector2UInt{10, 11}),
		std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, DISABLED_WideDestinationKeepsCornersAndFillsCenter)
{
	GTEST_SKIP() << "Requires a concrete 3x3 SpriteSheet and deterministic offscreen readback APIs not included in section 10.";
	// Intended assertion: very wide destinations preserve corner dimensions and fill every edge/center pixel without gaps.
}

TEST(NineSliceRenderCommandTest, DISABLED_TallDestinationKeepsCornersAndFillsCenter)
{
	GTEST_SKIP() << "Requires a concrete 3x3 SpriteSheet and deterministic offscreen readback APIs not included in section 10.";
	// Intended assertion: very tall destinations preserve corner dimensions and fill every edge/center pixel without gaps.
}
