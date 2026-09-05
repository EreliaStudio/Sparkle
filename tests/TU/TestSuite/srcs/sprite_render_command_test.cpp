#include <gtest/gtest.h>

#include <stdexcept>

#include "rendering/command/sprite_render_command.hpp"

namespace
{
	[[nodiscard]] spk::Rect2D destination()
	{
		return spk::Rect2D{.anchor = {4, 6}, .size = {32, 24}};
	}
}

TEST(SpriteRenderCommandTest, NullSpriteSheetIsRejected)
{
	EXPECT_THROW(
		(void)spk::SpriteRenderCommand(nullptr, spk::Vector2UInt{0, 0}, destination()),
		std::invalid_argument);
}

TEST(SpriteRenderCommandTest, DISABLED_FirstSpriteCoordinatesSelectFirstSection)
{
	GTEST_SKIP() << "Requires SpriteSheet construction and deterministic Texture/offscreen readback APIs not included in section 10.";
	// Intended assertion: coordinates {0,0} render the first sprite into the requested geometry.
}

TEST(SpriteRenderCommandTest, DISABLED_LastSpriteCoordinatesSelectLastSection)
{
	GTEST_SKIP() << "Requires SpriteSheet construction and deterministic Texture/offscreen readback APIs not included in section 10.";
	// Intended assertion: the maximum valid coordinates render the last sprite section.
}

TEST(SpriteRenderCommandTest, DISABLED_GeometryVariantsPreserveSelectedSprite)
{
	GTEST_SKIP() << "Requires SpriteSheet construction and deterministic offscreen readback APIs not included in section 10.";
	// Intended assertion: translated/scaled destination rectangles keep the same selected sprite content.
}

TEST(SpriteRenderCommandTest, DISABLED_DepthVariantsParticipateInDepthTesting)
{
	GTEST_SKIP() << "Requires SpriteSheet construction and deterministic offscreen depth testing not included in section 10.";
	// Intended assertion: overlapping sprites at different depth values obey the established depth convention.
}

TEST(SpriteRenderCommandTest, DISABLED_OutOfRangeCoordinatesPropagateSpriteSheetFailure)
{
	GTEST_SKIP() << "Requires a concrete SpriteSheet instance and its coordinate bounds API, which are transitive dependencies not included in section 10.";
	// Intended assertion: construction with coordinates outside the sheet propagates the SpriteSheet exception unchanged.
}
