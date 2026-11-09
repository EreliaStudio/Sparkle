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
			GLint _drawFramebuffer = 0;
			GLint _readFramebuffer = 0;
			std::array<GLint, 4> _viewport{};

		public:
			FramebufferBindingScope()
			{
				glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_drawFramebuffer);
				glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &_readFramebuffer);
				glGetIntegerv(GL_VIEWPORT, _viewport.data());
			}

			~FramebufferBindingScope()
			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(_drawFramebuffer));
				glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(_readFramebuffer));
				glViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
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
		FramebufferBindingScope binding;
		_framebuffer.activate(context);
		const auto &size = _framebuffer->size();
		glViewport(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y));
		_snapshot->execute(context);
	}
}
