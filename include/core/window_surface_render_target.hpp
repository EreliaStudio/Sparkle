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
		std::optional<Framebuffer::Handle> _framebuffer;

	public:
		RenderTarget() = default;
		explicit RenderTarget(Framebuffer::Handle framebuffer);

		[[nodiscard]] Type type() const noexcept;
		[[nodiscard]] Vector2UInt size(const Surface &surface) const;
		void activate(RenderContext &context) const;
	};
}
