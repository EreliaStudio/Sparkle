#include <gtest/gtest.h>

#include <stdexcept>

#include "rendering/command/viewport_uniform_render_command.hpp"

static_assert(spk::ViewportUniformRenderCommand::MatrixUBOBindingPoint == 0);

namespace
{
	[[nodiscard]] spk::Rect2D makeRect(spk::Vector2Int anchor, spk::Vector2UInt size)
	{
		return spk::Rect2D{.anchor = anchor, .size = size};
	}
}

TEST(ViewportUniformRenderCommandTest, RejectsZeroWidthViewport)
{
	EXPECT_THROW(
		(void)spk::ViewportUniformRenderCommand(makeRect({17, 29}, {0, 240})),
		std::invalid_argument);
}

TEST(ViewportUniformRenderCommandTest, RejectsZeroHeightViewport)
{
	EXPECT_THROW(
		(void)spk::ViewportUniformRenderCommand(makeRect({17, 29}, {320, 0})),
		std::invalid_argument);
}

TEST(ViewportUniformRenderCommandTest, RejectsViewportWithBothDimensionsEmpty)
{
	EXPECT_THROW(
		(void)spk::ViewportUniformRenderCommand(makeRect({17, 29}, {0, 0})),
		std::invalid_argument);
}

TEST(ViewportUniformRenderCommandTest, DISABLED_OffsetViewportBuildsAndBindsProjectionMatrix)
{
	GTEST_SKIP() << "Requires RenderContext plus UniformBuffer/OpenGL buffer introspection from the main repository, which are not included in section 10.";
	// Intended assertions:
	// - execute a non-empty offset viewport;
	// - binding point MatrixUBOBindingPoint contains the viewport projection matrix;
	// - local (0,0) and (width,height) map to the expected clip-space corners;
	// - physical viewport offset does not alter local projection coordinates.
}

TEST(ViewportUniformRenderCommandTest, DISABLED_RepeatedExecutionKeepsProjectionAndBindingStable)
{
	GTEST_SKIP() << "Requires RenderContext plus UniformBuffer/OpenGL buffer introspection from the main repository, which are not included in section 10.";
	// Intended assertion: execute the same command twice and verify identical matrix data and UBO binding on both executions.
}
