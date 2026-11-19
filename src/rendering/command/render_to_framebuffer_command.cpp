#include "rendering/command/render_to_framebuffer_command.hpp"

#include <stdexcept>
#include <utility>

#include "core/context/render_context.hpp"
#include "graphics/opengl/state.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	namespace
	{
		class FramebufferBindingScope final
		{
		private:
			OpenGL::State::Scope _openGLState;
			RenderContext &_context;
			Window::Surface::RenderTarget _previousTarget;

		public:
			FramebufferBindingScope(RenderContext &context, const Window::Surface::RenderTarget &target) :
				_openGLState(context),
				_context(context),
				_previousTarget(context.activeRenderTarget)
			{
				_context.activeRenderTarget = target;
				OpenGL::State::Scissor{.enabled = false}.apply(context);
			}

			~FramebufferBindingScope()
			{
				_context.activeRenderTarget = _previousTarget;
			}

			FramebufferBindingScope(const FramebufferBindingScope &) = delete;
			FramebufferBindingScope &operator=(const FramebufferBindingScope &) = delete;
		};
	}

	RenderToFramebufferCommand::RenderToFramebufferCommand(
		Framebuffer::Handle framebuffer,
		std::shared_ptr<const RenderSnapshot> snapshot) :
		_framebuffer(std::move(framebuffer)),
		_snapshot(std::move(snapshot))
	{
		if (!_framebuffer || !_snapshot)
		{
			throw std::invalid_argument("RenderToFramebufferCommand requires a framebuffer and a snapshot");
		}
	}

	void RenderToFramebufferCommand::execute(RenderContext &context) const
	{
		if (context.targetSurface == nullptr)
		{
			throw std::invalid_argument("Cannot render to a framebuffer without a target surface");
		}
		const Window::Surface::RenderTarget target(*context.targetSurface, _framebuffer);
		FramebufferBindingScope binding(context, target);
		target.activate(context);
		const Vector2UInt size = context.activeRenderTarget.size();
		OpenGL::State::Viewport{{0, 0, static_cast<int>(size.x), static_cast<int>(size.y)}}.apply(context);
		_snapshot->execute(context);
	}
}
