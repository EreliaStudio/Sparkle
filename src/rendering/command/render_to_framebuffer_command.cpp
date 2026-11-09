#include "rendering/command/render_to_framebuffer_command.hpp"

#include <GL/glew.h>

#include <array>
#include <stdexcept>
#include <utility>

#include "core/context/render_context.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	namespace
	{
		class FramebufferBindingScope final
		{
		private:
			RenderContext &_context;
			Window::Surface::RenderTarget _previousTarget;
			GLint _drawFramebuffer = 0;
			GLint _readFramebuffer = 0;
			std::array<GLint, 4> _viewport{};
			std::array<GLint, 4> _scissor{};
			GLboolean _scissorEnabled = GL_FALSE;

		public:
			FramebufferBindingScope(RenderContext &context, const Window::Surface::RenderTarget &target) :
				_context(context),
				_previousTarget(context.activeRenderTarget)
			{
				glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_drawFramebuffer);
				glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &_readFramebuffer);
				glGetIntegerv(GL_VIEWPORT, _viewport.data());
				glGetIntegerv(GL_SCISSOR_BOX, _scissor.data());
				_scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
				_context.activeRenderTarget = target;
				glDisable(GL_SCISSOR_TEST);
			}

			~FramebufferBindingScope()
			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(_drawFramebuffer));
				glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(_readFramebuffer));
				glViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
				glScissor(_scissor[0], _scissor[1], _scissor[2], _scissor[3]);
				if (_scissorEnabled == GL_TRUE)
				{
					glEnable(GL_SCISSOR_TEST);
				}
				else
				{
					glDisable(GL_SCISSOR_TEST);
				}
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
		const Vector2UInt size = context.renderTargetSize();
		glViewport(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y));
		_snapshot->execute(context);
	}
}
