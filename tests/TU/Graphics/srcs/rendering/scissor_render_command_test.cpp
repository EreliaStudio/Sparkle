#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstdint>

#include "rendering/command/scissor_render_command.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	[[nodiscard]] spk::Rect2D makeRect(spk::Vector2Int anchor, spk::Vector2UInt size)
	{
		return spk::Rect2D{.anchor = anchor, .size = size};
	}

	void expectPixel(const sparkle_test::FramebufferImage &image, spk::Vector2UInt position, const std::array<std::uint8_t, 4> &expected)
	{
		const std::uint8_t *actual = image.pixel(position);
		for (std::size_t channel = 0; channel < expected.size(); ++channel)
		{
			EXPECT_EQ(actual[channel], expected[channel]) << "channel " << channel;
		}
	}

	void clearRed()
	{
		::glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		::glClear(GL_COLOR_BUFFER_BIT);
	}
}

TEST(ScissorRenderCommandTest, StandardScissorCanBeConstructed)
{
	EXPECT_NO_THROW((void)spk::ScissorRenderCommand(makeRect({0, 0}, {640, 480})));
}

TEST(ScissorRenderCommandTest, OffsetScissorCanBeConstructed)
{
	EXPECT_NO_THROW((void)spk::ScissorRenderCommand(makeRect({23, 31}, {120, 90})));
}

TEST(ScissorRenderCommandTest, EmptyScissorCanBeConstructed)
{
	EXPECT_NO_THROW((void)spk::ScissorRenderCommand(makeRect({23, 31}, {0, 0})));
}

TEST(ScissorRenderCommandTest, OffsetScissorUpdatesOpenGLScissorState)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ScissorRenderCommand(makeRect({23, 31}, {120, 90})).execute(openGL.renderContext());

	std::array<GLint, 4> actual{};
	::glGetIntegerv(GL_SCISSOR_BOX, actual.data());
	EXPECT_EQ(actual, (std::array<GLint, 4>{23, 359, 120, 90}));
	EXPECT_EQ(::glIsEnabled(GL_SCISSOR_TEST), GL_TRUE);
}

TEST(ScissorRenderCommandTest, EmptyScissorRejectsAllPixels)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ScissorRenderCommand(makeRect({23, 31}, {0, 0})).execute(openGL.renderContext());
	clearRed();

	const auto image = openGL.capture();
	expectPixel(image, {23, 31}, {0, 0, 0, 0});
	expectPixel(image, {320, 240}, {0, 0, 0, 0});
}

TEST(ScissorRenderCommandTest, PartialScissorClipsPixelsToRequestedArea)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ScissorRenderCommand(makeRect({100, 120}, {200, 150})).execute(openGL.renderContext());
	clearRed();

	const auto image = openGL.capture();
	expectPixel(image, {99, 120}, {0, 0, 0, 0});
	expectPixel(image, {100, 120}, {255, 0, 0, 255});
	expectPixel(image, {299, 269}, {255, 0, 0, 255});
	expectPixel(image, {300, 269}, {0, 0, 0, 0});
}

TEST(ScissorRenderCommandTest, FullScissorAllowsAllTargetPixels)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ScissorRenderCommand(makeRect({0, 0}, {640, 480})).execute(openGL.renderContext());
	clearRed();

	const auto image = openGL.capture();
	expectPixel(image, {0, 0}, {255, 0, 0, 255});
	expectPixel(image, {639, 479}, {255, 0, 0, 255});
}
