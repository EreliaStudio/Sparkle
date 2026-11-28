#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>

#include "rendering/command/viewport_render_command.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	[[nodiscard]] spk::Rect2D makeRect(spk::Vector2Int anchor, spk::Vector2UInt size)
	{
		return spk::Rect2D{.anchor = anchor, .size = size};
	}

	void expectViewport(const std::array<GLint, 4> &expected)
	{
		std::array<GLint, 4> actual{};
		::glGetIntegerv(GL_VIEWPORT, actual.data());
		EXPECT_EQ(actual, expected);
	}
}

TEST(ViewportRenderCommandTest, StandardViewportCanBeConstructed)
{
	EXPECT_NO_THROW((void)spk::ViewportRenderCommand(makeRect({0, 0}, {640, 480})));
}

TEST(ViewportRenderCommandTest, OffsetViewportCanBeConstructed)
{
	EXPECT_NO_THROW((void)spk::ViewportRenderCommand(makeRect({37, 53}, {320, 240})));
}

TEST(ViewportRenderCommandTest, ZeroSizedViewportCanBeConstructed)
{
	EXPECT_NO_THROW((void)spk::ViewportRenderCommand(makeRect({10, 20}, {0, 0})));
}

TEST(ViewportRenderCommandTest, StandardViewportUpdatesOpenGLViewportState)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ViewportRenderCommand(makeRect({0, 0}, {640, 480})).execute(openGL.renderContext());

	expectViewport({0, 0, 640, 480});
}

TEST(ViewportRenderCommandTest, OffsetViewportUpdatesOpenGLViewportState)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ViewportRenderCommand(makeRect({37, 53}, {320, 240})).execute(openGL.renderContext());

	// Sparkle rectangles use a top-left origin; OpenGL uses a bottom-left origin.
	expectViewport({37, 187, 320, 240});
}

TEST(ViewportRenderCommandTest, ZeroSizedViewportUpdatesOpenGLViewportState)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ViewportRenderCommand(makeRect({10, 20}, {0, 0})).execute(openGL.renderContext());

	expectViewport({10, 460, 0, 0});
}

TEST(ViewportRenderCommandTest, ClippedViewportUsesRequestedOpenGLRectangle)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ViewportRenderCommand(makeRect({-20, -10}, {700, 520})).execute(openGL.renderContext());

	// OpenGL retains the requested rectangle and performs clipping while rasterizing.
	expectViewport({-20, -30, 700, 520});
}
