#include "core/window_surface_render_target.hpp"

#include <GL/glew.h>

#include <stdexcept>
#include <utility>

#include "core/context/render_context.hpp"

namespace spk
{
	Window::Surface::RenderTarget::RenderTarget(Surface &surface) noexcept :
		_surface(&surface)
	{
	}

	Window::Surface::RenderTarget::RenderTarget(Surface &surface, Framebuffer::Handle framebuffer) :
		_surface(&surface),
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

	Window::Surface &Window::Surface::RenderTarget::surface() const noexcept
	{
		return *_surface;
	}

	Vector2UInt Window::Surface::RenderTarget::size() const noexcept
	{
		return _framebuffer ? (*_framebuffer)->size() : _surface->geometry().size;
	}

	void Window::Surface::RenderTarget::activate(RenderContext &context) const
	{
		if (context.targetSurface != _surface)
		{
			throw std::invalid_argument("RenderTarget belongs to a different surface");
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

	Window::Surface::RenderTarget Window::Surface::defaultRenderTarget()
	{
		return RenderTarget{*this};
	}
}
