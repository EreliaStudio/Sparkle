#include <gtest/gtest.h>

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "exception.hpp"
#include "graphics/opengl/cube_map.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "rendering/command/clear_render_command.hpp"
#include "rendering/command/render_to_framebuffer_command.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	using Texture = spk::Texture;
	using Framebuffer = spk::Framebuffer;
	using CubeMap = spk::CubeMap;

	[[nodiscard]] Texture target(spk::Vector2UInt size, Texture::Format format)
	{
		return Texture::createRenderTarget(Texture::Target::Texture2D, size, format);
	}

	[[nodiscard]] std::shared_ptr<const spk::RenderSnapshot> clearSnapshot(
		spk::Color color,
		spk::ClearRenderCommand::Mask mask)
	{
		spk::RenderSnapshot::Builder builder;
		builder.renderPass({"clear", 0}).emplace<spk::ClearRenderCommand>(color, mask);
		return std::make_shared<spk::RenderSnapshot>(builder.build());
	}

	class CpuTexture final : public Texture
	{
	public:
		using Texture::setPixels;
	};
}

TEST(SharedFramebufferTest, ADepthHandleIsOwnedByBothFramebufferStates)
{
	auto depth = target({16, 12}, Texture::Format::Depth32F);
	auto colorA = target({16, 12}, Texture::Format::RGBA16F);
	auto colorB = target({16, 12}, Texture::Format::SRGB8_ALPHA8);
	Framebuffer opaque({16, 12}, std::vector{colorA.handle()}, depth.handle());
	Framebuffer transparent({16, 12}, std::vector{colorB.handle()}, depth.handle());

	ASSERT_TRUE(opaque.depthStencilAttachmentHandle());
	ASSERT_TRUE(transparent.depthStencilAttachmentHandle());
	EXPECT_EQ(opaque.depthStencilAttachmentHandle()->identifier(), depth.identifier());
	EXPECT_EQ(transparent.depthStencilAttachmentHandle()->identifier(), depth.identifier());
	EXPECT_EQ(
		std::addressof(opaque.depthStencilAttachmentHandle()->state()),
		std::addressof(transparent.depthStencilAttachmentHandle()->state()));
	EXPECT_EQ(opaque.colorAttachmentHandle(0)->format(), Texture::Format::RGBA16F);
	EXPECT_EQ(transparent.colorAttachmentHandle(0)->format(), Texture::Format::SRGB8_ALPHA8);
}

TEST(SharedFramebufferTest, SharedAttachmentsCannotBeResizedImplicitly)
{
	auto depth = target({9, 9}, Texture::Format::Depth24);
	Framebuffer framebuffer({9, 9}, std::vector<Texture::Handle>{}, depth.handle());
	auto old = framebuffer.handle();
	EXPECT_NO_THROW(framebuffer.resize({9, 9}));
	EXPECT_THROW(framebuffer.resize({10, 10}), spk::Exception);
	EXPECT_EQ(framebuffer.handle().identifier(), old.identifier());
	EXPECT_EQ(framebuffer.size(), spk::Vector2UInt(9, 9));
}

TEST(SharedFramebufferTest, LegacyResizePublishesIndependentStates)
{
	Framebuffer framebuffer({4, 3}, {Texture::Format::RGBA16F}, Texture::Format::Depth32F);
	const auto previous = framebuffer.handle();
	const auto previousColor = framebuffer.colorAttachmentHandle(0);
	const auto previousDepth = *framebuffer.depthStencilAttachmentHandle();
	framebuffer.resize({7, 5});

	EXPECT_NE(previous.identifier(), framebuffer.identifier());
	EXPECT_EQ(previous->size(), spk::Vector2UInt(4, 3));
	EXPECT_EQ(previous->colorAttachments()[0].texture.identifier(), previousColor.identifier());
	EXPECT_EQ(previous->depthStencilAttachment()->texture.identifier(), previousDepth.identifier());
	EXPECT_EQ(previousColor->size(), spk::Vector2UInt(4, 3));
	EXPECT_EQ(previousDepth->size(), spk::Vector2UInt(4, 3));
	EXPECT_EQ(framebuffer.colorAttachment(0).size(), spk::Vector2UInt(7, 5));
	EXPECT_NE(framebuffer.colorAttachmentHandle(0).identifier(), previousColor.identifier());
	EXPECT_NE(framebuffer.depthStencilAttachmentHandle()->identifier(), previousDepth.identifier());
}

TEST(SharedFramebufferTest, RejectsMissingInvalidMismatchedAndDuplicateAttachments)
{
	Texture::Handle missing;
	auto color = target({8, 8}, Texture::Format::RGBA16F);
	auto wrongSize = target({8, 9}, Texture::Format::RGBA);
	auto depth = target({8, 8}, Texture::Format::Depth32F);
	CpuTexture pixels;
	pixels.setPixels(nullptr, {8, 8}, Texture::Format::RGBA);
	pixels.validate();

	EXPECT_THROW((void)Framebuffer({8, 8}, std::vector<Texture::Handle>{}, std::nullopt), spk::Exception);
	EXPECT_THROW((void)Framebuffer({8, 8}, std::vector{missing}, std::nullopt), spk::Exception);
	EXPECT_THROW((void)Framebuffer({8, 8}, std::vector{wrongSize.handle()}, std::nullopt), spk::Exception);
	EXPECT_THROW((void)Framebuffer({8, 8}, std::vector{depth.handle()}, std::nullopt), spk::Exception);
	EXPECT_THROW((void)Framebuffer({8, 8}, std::vector{pixels.handle()}, std::nullopt), spk::Exception);
	EXPECT_THROW((void)Framebuffer({8, 8}, std::vector{color.handle(), color.handle()}, std::nullopt), spk::Exception);
	EXPECT_THROW((void)Framebuffer({8, 8}, std::vector{color.handle()}, color.handle()), spk::Exception);
	EXPECT_THROW((void)Framebuffer({8, 8}, std::vector{color.handle()}, missing), spk::Exception);
	EXPECT_NO_THROW((void)Framebuffer({8, 8}, std::vector{color.handle()}, depth.handle()));
	// Depth16 is not part of this API; a color texture cannot take the depth role.
	EXPECT_THROW((void)Framebuffer({8, 8}, std::vector<Texture::Handle>{}, color.handle()), spk::Exception);
}

TEST(SharedFramebufferTest, CubeMapProvidesOneResourceWithSixSquareFaces)
{
	using Target = Texture::Target;
	using Format = Texture::Format;
	EXPECT_THROW((void)Texture::createRenderTarget(Target::TextureCubeMap, {8, 9}, Format::Depth32F), spk::Exception);
	EXPECT_THROW((void)Texture::createRenderTarget(Target::Texture3D, {8, 8}, Format::Depth32F), spk::Exception);
	EXPECT_THROW((void)CubeMap(0), spk::Exception);
	CubeMap cube(8, Format::Depth32F);
	EXPECT_EQ(cube.faceSize(), 8u);
	EXPECT_EQ(cube.format(), Format::Depth32F);
	EXPECT_EQ(cube.handle()->target(), Target::TextureCubeMap);
	EXPECT_EQ(cube.handle()->contentSource(), Texture::ContentSource::RenderTarget);
	CubeMap color(8, Format::RGBA16F);
	EXPECT_EQ(color.format(), Format::RGBA16F);
}

TEST(SharedFramebufferTest, SharedDepthUsesTheSameNativeAttachmentOnOneSurface)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	auto depth = target({8, 8}, Texture::Format::Depth32F);
	auto opaqueColor = target({8, 8}, Texture::Format::RGBA16F);
	auto accum = target({8, 8}, Texture::Format::RGBA16F);
	auto reveal = target({8, 8}, Texture::Format::GreyLevel);
	Framebuffer opaque({8, 8}, std::vector{opaqueColor.handle()}, depth.handle());
	Framebuffer wboit({8, 8}, std::vector{accum.handle(), reveal.handle()}, depth.handle());

	const auto depthName = [](const Framebuffer &framebuffer, spk::RenderContext &context) {
		framebuffer.activate(context);
		GLint name = 0;
		glGetFramebufferAttachmentParameteriv(
			GL_FRAMEBUFFER,
			GL_DEPTH_ATTACHMENT,
			GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
			&name);
		return name;
	};
	const GLint first = depthName(opaque, gl.renderContext());
	const GLint second = depthName(wboit, gl.renderContext());
	EXPECT_GT(first, 0);
	EXPECT_EQ(first, second);
	EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(SharedFramebufferTest, FixedCubeFacesHaveIndependentDepthPixels)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	CubeMap cube(8, Texture::Format::Depth32F);
	EXPECT_THROW(
		(void)Framebuffer({8, 8}, std::vector<Texture::Handle>{}, cube.handle()),
		spk::Exception);
	std::vector<Framebuffer> framebuffers;
	for (unsigned int index = 0; index < 6; ++index)
	{
		const auto face = static_cast<CubeMap::Face>(index);
		framebuffers.emplace_back(
			spk::Vector2UInt{8, 8},
			std::vector<Framebuffer::Attachment>{},
			Framebuffer::Attachment{cube.handle(), face});
		auto &framebuffer = framebuffers.back();
		EXPECT_EQ(framebuffer.handle()->depthStencilAttachment()->face, face);
		EXPECT_EQ(framebuffer.depthStencilAttachmentHandle()->identifier(), cube.handle().identifier());
		glDepthMask(GL_TRUE);
		glClearDepth(0.1 * (index + 1));
		spk::RenderToFramebufferCommand(
			framebuffer.handle(),
			clearSnapshot({0, 0, 0, 1}, spk::ClearRenderCommand::Mask::Depth))
			.execute(gl.renderContext());
		framebuffer.activate(gl.renderContext());
		EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
		GLint attachedFace = GL_NONE;
		glGetFramebufferAttachmentParameteriv(
			GL_FRAMEBUFFER,
			GL_DEPTH_ATTACHMENT,
			GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE,
			&attachedFace);
		EXPECT_EQ(attachedFace, static_cast<GLint>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index));
	}
	cube.activate(gl.renderContext());
	for (unsigned int index = 0; index < 6; ++index)
	{
		std::array<float, 64> pixels{};
		glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels.data());
		EXPECT_NEAR(pixels[0], 0.1 * (index + 1), 0.0001);
	}
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	EXPECT_THROW(
		(void)Framebuffer(
			{8, 8},
			std::vector<Framebuffer::Attachment>{},
			Framebuffer::Attachment{cube.handle(), static_cast<CubeMap::Face>(9)}),
		spk::Exception);
	EXPECT_THROW(
		(void)Framebuffer(
			{8, 8},
			std::vector<Framebuffer::Attachment>{},
			Framebuffer::Attachment{target({8, 8}, Texture::Format::Depth32F).handle(), CubeMap::Face::PositiveX}),
		spk::Exception);
}

TEST(SharedFramebufferTest, CubemapColorFacesCanBeFramebufferAttachments)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	CubeMap cube(4, Texture::Format::RGBA16F);
	std::vector<Framebuffer> framebuffers;
	for (unsigned int index = 0; index < 6; ++index)
	{
		const auto face = static_cast<CubeMap::Face>(index);
		framebuffers.emplace_back(
			spk::Vector2UInt{4, 4},
			std::vector{Framebuffer::Attachment{cube.handle(), face}},
			std::nullopt);
		spk::RenderToFramebufferCommand(
			framebuffers.back().handle(),
			clearSnapshot(
				{static_cast<float>(index) / 6.0f, 0.5f, 0.0f, 1.0f},
				spk::ClearRenderCommand::Mask::Color))
			.execute(gl.renderContext());
		framebuffers.back().activate(gl.renderContext());
		EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
		GLint attachedFace = GL_NONE;
		glGetFramebufferAttachmentParameteriv(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0,
			GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE,
			&attachedFace);
		EXPECT_EQ(attachedFace, static_cast<GLint>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index));
	}
	cube.activate(gl.renderContext());
	for (unsigned int index = 0; index < 6; ++index)
	{
		std::array<float, 4 * 4 * 4> pixels{};
		glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, GL_RGBA, GL_FLOAT, pixels.data());
		EXPECT_NEAR(pixels[0], static_cast<float>(index) / 6.0f, 0.0005f);
	}
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(SharedFramebufferTest, OversizedTexturesAreRejectedBySurfaceCapabilities)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	GLint maximum = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum);
	ASSERT_GT(maximum, 0);
	if (maximum == std::numeric_limits<GLsizei>::max())
	{
		GTEST_SKIP() << "The GPU limit cannot be exceeded within GLsizei";
	}
	auto color = target({static_cast<unsigned int>(maximum) + 1, 1}, Texture::Format::RGBA);
	Framebuffer framebuffer(color.size(), std::vector{color.handle()}, std::nullopt);
	EXPECT_THROW(framebuffer.activate(gl.renderContext()), spk::Exception);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(SharedFramebufferTest, CpuHdrStorageAndInvalidTargetDimensionsAreRejected)
{
	using Format = Texture::Format;
	using Target = Texture::Target;
	CpuTexture cpu;
	EXPECT_THROW(cpu.setPixels(nullptr, {1, 1}, Format::RGBA16F), std::invalid_argument);
	EXPECT_THROW((void)Texture::createRenderTarget(Target::Texture2D, {0, 1}, Format::RGBA16F), spk::Exception);
	EXPECT_THROW((void)Texture::createRenderTarget(Target::Texture2D, {1, 0}, Format::SRGB8_ALPHA8), spk::Exception);
	EXPECT_THROW((void)Texture::createRenderTarget(Target::Texture2D, {1, 1}, Format::Error), spk::Exception);

	auto hdr = Texture::createRenderTarget(Target::Texture2D, {8, 8}, Format::RGBA16F);
	auto srgb = Texture::createRenderTarget(Target::Texture2D, {8, 8}, Format::SRGB8_ALPHA8);
	EXPECT_EQ(hdr.handle()->contentSource(), Texture::ContentSource::RenderTarget);
	EXPECT_EQ(srgb.handle()->contentSource(), Texture::ContentSource::RenderTarget);
	EXPECT_EQ(Texture::formatDescriptor(Format::RGBA16F).internalFormat, GL_RGBA16F);
	EXPECT_EQ(Texture::formatDescriptor(Format::SRGB8_ALPHA8).internalFormat, GL_SRGB8_ALPHA8);
}

TEST(SharedFramebufferTest, FailedResizePreservesPreviouslyCapturedVersions)
{
	Framebuffer framebuffer({8, 8}, {Texture::Format::RGBA16F}, Texture::Format::Depth32F);
	auto oldFramebuffer = framebuffer.handle();
	auto oldColor = framebuffer.colorAttachmentHandle(0);
	auto oldDepth = *framebuffer.depthStencilAttachmentHandle();
	EXPECT_THROW(framebuffer.resize({0, 9}), std::invalid_argument);
	EXPECT_EQ(framebuffer.handle().identifier(), oldFramebuffer.identifier());
	EXPECT_EQ(framebuffer.colorAttachmentHandle(0).identifier(), oldColor.identifier());
	EXPECT_EQ(framebuffer.depthStencilAttachmentHandle()->identifier(), oldDepth.identifier());
	EXPECT_EQ(oldFramebuffer->size(), spk::Vector2UInt(8, 8));
}

TEST(SharedFramebufferTest, ExcessColorAttachmentsAreRejectedByActualGlLimits)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	GLint maxDraw = 0;
	GLint maxColors = 0;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDraw);
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColors);
	const auto limit = std::min(maxDraw, maxColors);
	ASSERT_GT(limit, 0);
	std::vector<Texture::Handle> handles;
	for (GLint index = 0; index <= limit; ++index)
	{
		handles.push_back(target({1, 1}, Texture::Format::RGBA).handle());
	}
	Framebuffer framebuffer({1, 1}, std::move(handles), std::nullopt);
	EXPECT_THROW(framebuffer.activate(gl.renderContext()), spk::Exception);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}
