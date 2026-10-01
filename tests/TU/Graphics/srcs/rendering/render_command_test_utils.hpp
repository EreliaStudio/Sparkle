#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "geometry/color_mesh_2d.hpp"
#include "geometry/texture_mesh_2d.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "rendering/command/clear_render_command.hpp"
#include "rendering/command/viewport_render_command.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace render_command_test
{
	class Texture final : public spk::Texture
	{
	public:
		Texture(
			const spk::Vector2UInt &size,
			std::span<const std::uint8_t> pixels,
			Format format = Format::RGBA)
		{
			setPixels(pixels, size, format);
			validate();
		}
	};

	class Target
	{
	private:
		sparkle_test::OpenGLTestContext &_openGL;
		spk::Framebuffer _framebuffer;
		spk::Rect2D _viewport;

	public:
		explicit Target(spk::Vector2UInt size = {64, 64}) :
			_openGL(sparkle_test::OpenGLTestContext::instance()),
			_framebuffer(size),
			_viewport{.anchor = {0, 0}, .size = size}
		{
			_openGL.reset();
			_openGL.setGeometry(_viewport);
			_framebuffer.activate(_openGL.renderContext());
			spk::ViewportRenderCommand(_viewport).execute(_openGL.renderContext());
			spk::ViewportUniformRenderCommand(_viewport).execute(_openGL.renderContext());
			glDisable(GL_SCISSOR_TEST);
		}

		[[nodiscard]] spk::RenderContext &context()
		{
			return _openGL.renderContext();
		}

		[[nodiscard]] const spk::Rect2D &viewport() const noexcept
		{
			return _viewport;
		}

		void clear(spk::Color color = {0, 0, 0, 0})
		{
			spk::ClearRenderCommand(color, spk::ClearRenderCommand::Mask::All).execute(context());
		}

		[[nodiscard]] sparkle_test::FramebufferImage capture()
		{
			return _openGL.capture();
		}
	};

	inline spk::ColorMesh2D colorQuad(const spk::Rect2D &geometry, const spk::Color &color, float depth = 0.0f)
	{
		spk::ColorMesh2D::Builder builder;
		const float left = static_cast<float>(geometry.x);
		const float top = static_cast<float>(geometry.y);
		const float right = static_cast<float>(geometry.x + geometry.width);
		const float bottom = static_cast<float>(geometry.y + geometry.height);
		builder.addShape(
			{{left, top}, depth, color},
			{{left, bottom}, depth, color},
			{{right, bottom}, depth, color},
			{{right, top}, depth, color});
		return std::move(builder).build();
	}

	inline spk::TextureMesh2D textureQuad(
		const spk::Rect2D &geometry,
		const spk::Texture::Section &section = spk::Texture::Section::whole,
		float depth = 0.0f)
	{
		spk::TextureMesh2D::Builder builder;
		const float left = static_cast<float>(geometry.x);
		const float top = static_cast<float>(geometry.y);
		const float right = static_cast<float>(geometry.x + geometry.width);
		const float bottom = static_cast<float>(geometry.y + geometry.height);
		const float uvLeft = section.anchor.x;
		const float uvTop = section.anchor.y;
		const float uvRight = section.anchor.x + section.size.x;
		const float uvBottom = section.anchor.y + section.size.y;
		builder.addShape(
			{{left, top}, depth, {uvLeft, uvTop}},
			{{left, bottom}, depth, {uvLeft, uvBottom}},
			{{right, bottom}, depth, {uvRight, uvBottom}},
			{{right, top}, depth, {uvRight, uvTop}});
		return std::move(builder).build();
	}

	inline std::array<std::uint8_t, 4> pixel(
		const sparkle_test::FramebufferImage &image,
		spk::Vector2UInt position)
	{
		const auto *value = image.pixel(position);
		return {value[0], value[1], value[2], value[3]};
	}

	inline std::size_t countColor(
		const sparkle_test::FramebufferImage &image,
		const std::array<std::uint8_t, 4> &color,
		std::uint8_t tolerance = 0)
	{
		std::size_t result = 0;
		for (std::size_t index = 0; index < image.pixels.size(); index += 4)
		{
			bool matches = true;
			for (std::size_t channel = 0; channel < color.size(); ++channel)
			{
				const int difference = static_cast<int>(image.pixels[index + channel]) - color[channel];
				matches &= difference >= -tolerance && difference <= tolerance;
			}
			result += matches;
		}
		return result;
	}

	inline std::vector<std::uint8_t> encodedPpm(
		const spk::Vector2UInt &size,
		std::span<const std::uint8_t> rgbPixels)
	{
		const std::string header = "P6\n" + std::to_string(size.x) + " " + std::to_string(size.y) + "\n255\n";
		std::vector<std::uint8_t> result(header.begin(), header.end());
		result.insert(result.end(), rgbPixels.begin(), rgbPixels.end());
		return result;
	}
}
