#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "graphics/opengl/framebuffer.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

static_assert(!std::is_copy_constructible_v<spk::Framebuffer>);
static_assert(std::is_move_constructible_v<spk::Framebuffer>);
static_assert(!std::is_copy_assignable_v<spk::Framebuffer>);
static_assert(!std::is_move_assignable_v<spk::Framebuffer>);

TEST(FramebufferTest, DefaultAttachmentsExposeRenderTargetTextures)
{
	spk::Framebuffer framebuffer({16, 12});

	EXPECT_EQ(framebuffer.size(), spk::Vector2UInt(16, 12));
	ASSERT_EQ(framebuffer.colorAttachmentCount(), 1u);
	EXPECT_EQ(framebuffer.colorAttachment(0).size(), framebuffer.size());
	EXPECT_EQ(framebuffer.colorAttachment(0).format(), spk::Texture::Format::RGBA);
	EXPECT_TRUE(framebuffer.colorAttachment(0).isRenderTarget());
	ASSERT_NE(framebuffer.depthStencilAttachment(), nullptr);
	EXPECT_EQ(framebuffer.depthStencilAttachment()->size(), framebuffer.size());
	EXPECT_EQ(framebuffer.depthStencilAttachment()->format(), spk::Texture::Format::Depth24Stencil8);
	EXPECT_TRUE(framebuffer.depthStencilAttachment()->isRenderTarget());
	EXPECT_THROW((void)framebuffer.colorAttachment(1), std::out_of_range);
}

TEST(FramebufferTest, ConstructorRejectsInvalidSizesFormatsAndMissingAttachments)
{
	using Format = spk::Texture::Format;

	EXPECT_THROW((void)spk::Framebuffer({0, 1}), std::invalid_argument);
	EXPECT_THROW((void)spk::Framebuffer({1, 0}), std::invalid_argument);
	EXPECT_THROW(
		(void)spk::Framebuffer({static_cast<unsigned int>(std::numeric_limits<GLsizei>::max()) + 1u, 1}),
		std::overflow_error);
	EXPECT_THROW(
		(void)spk::Framebuffer({1, 1}, std::vector{Format::Depth24}, std::nullopt),
		std::invalid_argument);
	EXPECT_THROW(
		(void)spk::Framebuffer({1, 1}, std::vector<Format>{}, Format::RGBA),
		std::invalid_argument);
	EXPECT_THROW(
		(void)spk::Framebuffer({1, 1}, std::vector<Format>{}, std::nullopt),
		std::invalid_argument);
}

TEST(FramebufferTest, ActivationCreatesCompleteMultipleColorFramebuffer)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Framebuffer framebuffer(
		{8, 6},
		{spk::Texture::Format::RGBA, spk::Texture::Format::RGB},
		spk::Texture::Format::Depth24Stencil8);

	framebuffer.activate(openGL.renderContext());

	GLint identifier = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &identifier);
	EXPECT_NE(identifier, 0);
	EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);

	for (GLenum attachment : {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_DEPTH_STENCIL_ATTACHMENT})
	{
		GLint type = GL_NONE;
		GLint name = 0;
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
		EXPECT_EQ(type, GL_TEXTURE);
		EXPECT_NE(name, 0);
	}

	GLint drawBuffer = GL_NONE;
	glGetIntegerv(GL_DRAW_BUFFER1, &drawBuffer);
	EXPECT_EQ(drawBuffer, GL_COLOR_ATTACHMENT1);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(FramebufferTest, DepthOnlyFramebufferDisablesColorReadAndDrawBuffers)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Framebuffer framebuffer(
		{8, 6},
		std::vector<spk::Texture::Format>{},
		spk::Texture::Format::Depth32F);

	framebuffer.activate(openGL.renderContext());

	EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
	GLint drawBuffer = -1;
	GLint readBuffer = -1;
	glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
	glGetIntegerv(GL_READ_BUFFER, &readBuffer);
	EXPECT_EQ(drawBuffer, GL_NONE);
	EXPECT_EQ(readBuffer, GL_NONE);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(FramebufferTest, ClearAndReadbackUseTheColorAttachment)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Framebuffer framebuffer({2, 2}, {spk::Texture::Format::RGBA}, std::nullopt);
	framebuffer.activate(openGL.renderContext());

	glViewport(0, 0, 2, 2);
	glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	std::array<std::uint8_t, 4> pixel{};
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());

	EXPECT_NEAR(pixel[0], 64, 1);
	EXPECT_NEAR(pixel[1], 128, 1);
	EXPECT_NEAR(pixel[2], 191, 1);
	EXPECT_EQ(pixel[3], 255);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(FramebufferTest, ResizeReallocatesAttachmentsAndInvalidatesOnce)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Framebuffer framebuffer({4, 3});
	const auto generation = framebuffer.generation();

	framebuffer.resize({4, 3});
	EXPECT_EQ(framebuffer.generation(), generation);
	framebuffer.resize({7, 5});
	EXPECT_EQ(framebuffer.generation(), generation + 1);
	EXPECT_EQ(framebuffer.size(), spk::Vector2UInt(7, 5));
	EXPECT_EQ(framebuffer.colorAttachment(0).size(), framebuffer.size());
	EXPECT_EQ(framebuffer.depthStencilAttachment()->size(), framebuffer.size());

	framebuffer.activate(openGL.renderContext());
	framebuffer.colorAttachment(0).activate(openGL.renderContext());
	GLint width = 0;
	GLint height = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	EXPECT_EQ(width, 7);
	EXPECT_EQ(height, 5);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(FramebufferTest, MoveConstructionPreservesResourceAndAttachments)
{
	spk::Framebuffer source({3, 2}, {spk::Texture::Format::RGB}, std::nullopt);
	const auto identifier = source.identifier();
	const auto colorIdentifier = source.colorAttachment(0).identifier();

	spk::Framebuffer moved(std::move(source));

	EXPECT_EQ(moved.identifier(), identifier);
	EXPECT_EQ(moved.size(), spk::Vector2UInt(3, 2));
	ASSERT_EQ(moved.colorAttachmentCount(), 1u);
	EXPECT_EQ(moved.colorAttachment(0).identifier(), colorIdentifier);
}
