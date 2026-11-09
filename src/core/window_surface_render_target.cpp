#include "core/window_surface_render_target.hpp"

#include <GL/glew.h>

#include <stdexcept>
#include <utility>

#include "core/context/render_context.hpp"

namespace spk
{
	Window::Surface::RenderTarget::RenderTarget(Framebuffer::Handle framebuffer) :
		_framebuffer(std::move(framebuffer))
	{
		if (!*_framebuffer)
		{
			throw std::invalid_argument("RenderTarget framebuffer handle cannot be empty");
		}
	}

	Window::Surface::RenderTarget::Type Window::Surface::RenderTarget::type() const noexcept
	{
		return _framebuffer ? Type::Framebuffer : Type::Default;
	}

	Vector2UInt Window::Surface::RenderTarget::size(const Surface &surface) const
	{
		return _framebuffer ? (*_framebuffer)->size() : surface.geometry().size;
	}

	void Window::Surface::RenderTarget::activate(RenderContext &context) const
	{
		if (context.targetSurface == nullptr)
		{
			throw std::invalid_argument("Cannot activate a render target without a surface");
		}
		if (_framebuffer)
		{
			_framebuffer->activate(context);
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	Window::Surface::RenderTarget Window::Surface::defaultRenderTarget() const
	{
		return RenderTarget{};
	}
}
