#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstdint>

#include "rendering/command/clear_render_command.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	struct Attachments
	{
		std::array<std::uint8_t, 4> color{};
		float depth = 0.0f;
		std::uint8_t stencil = 0;
	};

	Attachments readAttachments()
	{
		Attachments result;
		::glReadPixels(10, 10, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, result.color.data());
		::glReadPixels(10, 10, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &result.depth);
		::glReadPixels(10, 10, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &result.stencil);
		return result;
	}

	void seedAttachments()
	{
		::glClearColor(0.2f, 0.4f, 0.6f, 0.8f);
		::glClearDepth(0.25);
		::glClearStencil(7);
		::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
}

TEST(ClearRenderCommandTest, ColorMaskChangesColorAndLeavesDepthAndStencilUntouched)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	seedAttachments();
	spk::ClearRenderCommand({1.0f, 0.0f, 0.5f, 1.0f}, spk::ClearRenderCommand::Mask::Color).execute(openGL.renderContext());
	const auto actual = readAttachments();
	EXPECT_EQ(actual.color[0], 255);
	EXPECT_EQ(actual.color[1], 0);
	EXPECT_NEAR(actual.color[2], 128, 1);
	EXPECT_EQ(actual.color[3], 255);
	EXPECT_NEAR(actual.depth, 0.25f, 1.0e-5f);
	EXPECT_EQ(actual.stencil, 7);
}

TEST(ClearRenderCommandTest, DepthAndStencilMasksLeaveColorUntouched)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	seedAttachments();
	::glClearDepth(0.75);
	::glClearStencil(3);
	spk::ClearRenderCommand({1.0f, 0.0f, 0.0f, 1.0f},
		spk::ClearRenderCommand::Mask::Depth | spk::ClearRenderCommand::Mask::Stencil).execute(openGL.renderContext());
	const auto actual = readAttachments();
	EXPECT_EQ(actual.color, (std::array<std::uint8_t, 4>{51, 102, 153, 204}));
	EXPECT_NEAR(actual.depth, 0.75f, 1.0e-5f);
	EXPECT_EQ(actual.stencil, 3);
}

TEST(ClearRenderCommandTest, NoneMaskChangesNeitherAttachmentsNorClearColorState)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	seedAttachments();
	std::array<GLfloat, 4> before{};
	::glGetFloatv(GL_COLOR_CLEAR_VALUE, before.data());
	spk::ClearRenderCommand({1.0f, 0.0f, 0.0f, 1.0f}, spk::ClearRenderCommand::Mask::None).execute(openGL.renderContext());
	std::array<GLfloat, 4> after{};
	::glGetFloatv(GL_COLOR_CLEAR_VALUE, after.data());
	EXPECT_EQ(after, before);
	const auto actual = readAttachments();
	EXPECT_EQ(actual.color, (std::array<std::uint8_t, 4>{51, 102, 153, 204}));
	EXPECT_NEAR(actual.depth, 0.25f, 1.0e-5f);
	EXPECT_EQ(actual.stencil, 7);
}

TEST(ClearRenderCommandTest, AllMaskClearsEveryAttachment)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	seedAttachments();
	::glClearDepth(0.5);
	::glClearStencil(11);
	spk::ClearRenderCommand({0.0f, 1.0f, 0.0f, 0.25f}, spk::ClearRenderCommand::Mask::All).execute(openGL.renderContext());
	const auto actual = readAttachments();
	EXPECT_EQ(actual.color, (std::array<std::uint8_t, 4>{0, 255, 0, 64}));
	EXPECT_NEAR(actual.depth, 0.5f, 1.0e-5f);
	EXPECT_EQ(actual.stencil, 11);
}
