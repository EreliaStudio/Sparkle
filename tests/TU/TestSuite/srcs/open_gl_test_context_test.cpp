#include <gtest/gtest.h>

#include <GL/glew.h>

#include <filesystem>
#include <stdexcept>

#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"

TEST(OpenGLTestContextTest, ReusesOneReadyContextAndRestoresDeterministicState)
{
	auto &first = sparkle_test::OpenGLTestContext::instance();
	auto &second = sparkle_test::OpenGLTestContext::instance();

	EXPECT_EQ(&first, &second);
	EXPECT_EQ(first.surface().lifeCycle(), spk::Window::LifeCycle::Ready);
	EXPECT_EQ(first.renderContext().targetSurface, &first.surface());

	::glEnable(GL_SCISSOR_TEST);
	::glViewport(1, 2, 3, 4);
	first.reset();

	EXPECT_EQ(::glIsEnabled(GL_SCISSOR_TEST), GL_FALSE);
	GLint viewport[4]{};
	::glGetIntegerv(GL_VIEWPORT, viewport);
	EXPECT_EQ(viewport[0], 0);
	EXPECT_EQ(viewport[1], 0);
	EXPECT_EQ(viewport[2], 640);
	EXPECT_EQ(viewport[3], 480);
}

TEST(OpenGLTestContextTest, CapturesPixelsWithTopLeftCoordinates)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	::glEnable(GL_SCISSOR_TEST);
	::glScissor(0, 479, 1, 1);
	::glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
	::glClear(GL_COLOR_BUFFER_BIT);

	const auto image = openGL.capture();
	ASSERT_EQ(image.size, (spk::Vector2UInt{640, 480}));
	EXPECT_EQ(image.pixel({0, 0})[1], 255);
	EXPECT_EQ(image.pixel({0, 479})[1], 0);
	EXPECT_THROW((void)image.pixel({640, 0}), std::out_of_range);
}
