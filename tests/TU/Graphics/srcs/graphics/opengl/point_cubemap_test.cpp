#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <vector>

#include "graphics/opengl/cube_map.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "graphics/opengl/sampler.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

TEST(PointCubemapTest, CreatesSixIndependentDepth32FFacesAndPreservesThem)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset({32, 32});
	spk::CubeMap map(32);
	EXPECT_EQ(map.faceSize(), 32u);
	EXPECT_EQ(map.format(), spk::Texture::Format::Depth32F);
	ASSERT_EQ(map.handle()->target(), spk::Texture::Target::TextureCubeMap);
	std::array<spk::Framebuffer, 6> frames;
	for (std::size_t i = 0; i < frames.size(); ++i)
	{
		frames[i] = spk::Framebuffer(
			{32, 32},
			std::vector<spk::Framebuffer::RenderAttachment>{},
			spk::Framebuffer::RenderAttachment(map.handle(), static_cast<spk::CubeMap::Face>(i)));
		frames[i].activate(gl.renderContext());
		EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
		glClearDepth(0.1 + static_cast<double>(i) * 0.1);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	for (std::size_t i = 0; i < frames.size(); ++i)
	{
		frames[i].activate(gl.renderContext());
		float depth = -1.0f;
		glReadPixels(16, 16, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
		EXPECT_NEAR(depth, 0.1f + static_cast<float>(i) * 0.1f, 0.0001f);
	}
	spk::Sampler sampler(
		1, spk::Sampler::Filtering::Nearest, spk::Sampler::Wrap::ClampToEdge);
	sampler.setTexture(map.handle());
	sampler.setDepthComparison(spk::Sampler::DepthComparison::LessEqual);
	sampler.validate();
	sampler.activate(gl.renderContext());
	GLint bound = 0;
	glGetIntegeri_v(GL_SAMPLER_BINDING, 1, &bound);
	ASSERT_NE(bound, 0);
	GLint filtering = -1;
	glGetSamplerParameteriv(static_cast<GLuint>(bound), GL_TEXTURE_MIN_FILTER, &filtering);
	EXPECT_EQ(filtering, GL_NEAREST);
	EXPECT_NO_THROW(gl.requireNoErrors());
	gl.reset();
}

TEST(PointCubemapTest, RejectsInvalidFacesAndSizeBeforeGraphicsAllocation)
{
	using Attachment = spk::Framebuffer::RenderAttachment;
	spk::CubeMap map(64);
	EXPECT_THROW((void)spk::CubeMap(0), spk::Exception);
	EXPECT_THROW(
		(void)spk::Framebuffer(
			{64, 64}, std::vector<Attachment>{}, Attachment(map.handle(), static_cast<spk::CubeMap::Face>(6))),
		spk::Exception);
	EXPECT_THROW(
		(void)spk::Framebuffer(
			{32, 32}, std::vector<Attachment>{}, Attachment(map.handle(), spk::CubeMap::Face::PositiveX)),
		spk::Exception);
	EXPECT_THROW(
		(void)spk::Framebuffer({64, 64}, std::vector<Attachment>{}, Attachment(map.handle())),
		spk::Exception);
}

TEST(PointCubemapTest, DefaultDepthFormatHas1024TexelFaces)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset({32, 32});
	spk::CubeMap map(1024);
	EXPECT_EQ(map.faceSize(), 1024u);
	EXPECT_EQ(map.handle()->size(), spk::Vector2UInt(1024, 1024));
	EXPECT_EQ(map.handle()->format(), spk::Texture::Format::Depth32F);
	map.activate(gl.renderContext());
	for (std::size_t face = 0; face < 6; ++face)
	{
		const GLenum target = static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face);
		GLint width = 0;
		GLint height = 0;
		GLint format = GL_NONE;
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, &height);
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
		EXPECT_EQ(width, 1024);
		EXPECT_EQ(height, 1024);
		EXPECT_EQ(format, GL_DEPTH_COMPONENT32F);
	}
	EXPECT_NO_THROW(gl.requireNoErrors());
	gl.reset();
}
