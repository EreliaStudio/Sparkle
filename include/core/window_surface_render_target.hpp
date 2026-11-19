#pragma once

#include <optional>

#include "core/window.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "math/vector2.hpp"

namespace spk
{
	class Window::Surface::RenderTarget final
	{
	public:
		enum class Type
		{
			Default,
			Framebuffer
		};

	private:
		Surface *_surface;
		std::optional<Framebuffer::Handle> _framebuffer;

	public:
		explicit RenderTarget(Surface &surface) noexcept;
		RenderTarget(Surface &surface, Framebuffer::Handle framebuffer);

		[[nodiscard]] Type type() const noexcept;
		[[nodiscard]] Surface &surface() const noexcept;
		[[nodiscard]] Vector2UInt size() const noexcept;
		void activate(RenderContext &context) const;
	};
}
