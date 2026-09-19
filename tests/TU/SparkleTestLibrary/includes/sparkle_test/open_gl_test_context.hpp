#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "core/context/render_context.hpp"
#include "graphics/color.hpp"
#include "math/vector2.hpp"

namespace sparkle_test
{
	struct FramebufferImage
	{
		spk::Vector2UInt size;
		std::vector<std::uint8_t> pixels;

		[[nodiscard]] const std::uint8_t *pixel(spk::Vector2UInt position) const;
	};

	/**
	 * Owns the hidden native window and OpenGL surface shared by GPU tests.
	 *
	 * The instance is created lazily, so CPU-only test runs do not initialize
	 * OpenGL. Calls must be made from the test runner thread because a WGL
	 * context can only be current on one thread at a time.
	 */
	class OpenGLTestContext final
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> _impl;

		OpenGLTestContext();

	public:
		OpenGLTestContext(const OpenGLTestContext &) = delete;
		OpenGLTestContext(OpenGLTestContext &&) = delete;
		~OpenGLTestContext();

		OpenGLTestContext &operator=(const OpenGLTestContext &) = delete;
		OpenGLTestContext &operator=(OpenGLTestContext &&) = delete;

		[[nodiscard]] static OpenGLTestContext &instance();

		void makeCurrent();
		void reset();
		void setGeometry(const spk::Rect2D &geometry);

		[[nodiscard]] spk::RenderContext &renderContext();
		[[nodiscard]] spk::Window::Surface &surface();
		[[nodiscard]] FramebufferImage capture();
		void save(const std::filesystem::path &path);
	};
}
