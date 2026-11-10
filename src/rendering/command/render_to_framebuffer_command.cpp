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
			struct BlendState
			{
				GLboolean enabled = GL_FALSE;
				GLint srcRGB = GL_ONE, dstRGB = GL_ZERO;
				GLint srcAlpha = GL_ONE, dstAlpha = GL_ZERO;
				GLint equationRGB = GL_FUNC_ADD, equationAlpha = GL_FUNC_ADD;

				void capture(GLuint index)
				{
					enabled = glIsEnabledi(GL_BLEND, index);
					glGetIntegeri_v(GL_BLEND_SRC_RGB, index, &srcRGB);
					glGetIntegeri_v(GL_BLEND_DST_RGB, index, &dstRGB);
					glGetIntegeri_v(GL_BLEND_SRC_ALPHA, index, &srcAlpha);
					glGetIntegeri_v(GL_BLEND_DST_ALPHA, index, &dstAlpha);
					glGetIntegeri_v(GL_BLEND_EQUATION_RGB, index, &equationRGB);
					glGetIntegeri_v(GL_BLEND_EQUATION_ALPHA, index, &equationAlpha);
				}

				void restore(GLuint index) const
				{
					enabled ? glEnablei(GL_BLEND, index) : glDisablei(GL_BLEND, index);
					glBlendFuncSeparatei(index, srcRGB, dstRGB, srcAlpha, dstAlpha);
					glBlendEquationSeparatei(index, equationRGB, equationAlpha);
				}
			};

			RenderContext &_context;
			Window::Surface::RenderTarget _previousTarget;
			GLint _drawFramebuffer = 0, _readFramebuffer = 0;
			GLint _depthFunc = GL_LESS, _cullFace = GL_BACK, _frontFace = GL_CCW;
			GLint _currentProgram = 0, _vertexArray = 0, _activeTexture = GL_TEXTURE0;
			GLint _stencilWriteMask = 0;
			std::array<GLint, 4> _viewport{}, _scissor{};
			std::array<GLboolean, 4> _colorMask{};
			std::array<GLfloat, 4> _clearColor{};
			std::array<BlendState, 2> _blend{};
			GLboolean _scissorEnabled = GL_FALSE, _depthEnabled = GL_FALSE;
			GLboolean _depthWrite = GL_TRUE, _cullEnabled = GL_FALSE, _srgbEnabled = GL_FALSE;

			static void _restoreToggle(GLenum capability, GLboolean enabled)
			{
				enabled == GL_TRUE ? glEnable(capability) : glDisable(capability);
			}

			void _capture()
			{
				glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_drawFramebuffer);
				glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &_readFramebuffer);
				glGetIntegerv(GL_VIEWPORT, _viewport.data());
				glGetIntegerv(GL_SCISSOR_BOX, _scissor.data());
				glGetIntegerv(GL_DEPTH_FUNC, &_depthFunc);
				glGetIntegerv(GL_CULL_FACE_MODE, &_cullFace);
				glGetIntegerv(GL_FRONT_FACE, &_frontFace);
				glGetIntegerv(GL_CURRENT_PROGRAM, &_currentProgram);
				glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &_vertexArray);
				glGetIntegerv(GL_ACTIVE_TEXTURE, &_activeTexture);
				glGetIntegerv(GL_STENCIL_WRITEMASK, &_stencilWriteMask);
				glGetBooleanv(GL_COLOR_WRITEMASK, _colorMask.data());
				glGetBooleanv(GL_DEPTH_WRITEMASK, &_depthWrite);
				glGetFloatv(GL_COLOR_CLEAR_VALUE, _clearColor.data());
				_scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
				_depthEnabled = glIsEnabled(GL_DEPTH_TEST);
				_cullEnabled = glIsEnabled(GL_CULL_FACE);
				_srgbEnabled = glIsEnabled(GL_FRAMEBUFFER_SRGB);
				for (GLuint index = 0; index < _blend.size(); ++index)
				{
					_blend[index].capture(index);
				}
			}

			void _restore() noexcept
			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(_drawFramebuffer));
				glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(_readFramebuffer));
				glViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
				glScissor(_scissor[0], _scissor[1], _scissor[2], _scissor[3]);
				glDepthFunc(_depthFunc);
				glDepthMask(_depthWrite);
				glColorMask(_colorMask[0], _colorMask[1], _colorMask[2], _colorMask[3]);
				glStencilMask(static_cast<GLuint>(_stencilWriteMask));
				glCullFace(_cullFace);
				glFrontFace(_frontFace);
				glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
				_restoreToggle(GL_SCISSOR_TEST, _scissorEnabled);
				_restoreToggle(GL_DEPTH_TEST, _depthEnabled);
				_restoreToggle(GL_CULL_FACE, _cullEnabled);
				_restoreToggle(GL_FRAMEBUFFER_SRGB, _srgbEnabled);
				for (GLuint index = 0; index < _blend.size(); ++index)
				{
					_blend[index].restore(index);
				}
				glUseProgram(static_cast<GLuint>(_currentProgram));
				glBindVertexArray(static_cast<GLuint>(_vertexArray));
				glActiveTexture(static_cast<GLenum>(_activeTexture));
				_context.activeRenderTarget = _previousTarget;
			}

		public:
			FramebufferBindingScope(RenderContext &context, const Window::Surface::RenderTarget &target) :
				_context(context),
				_previousTarget(context.activeRenderTarget)
			{
				_capture();
				_context.activeRenderTarget = target;
				glDisable(GL_SCISSOR_TEST);
			}

			~FramebufferBindingScope()
			{
				_restore();
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
		glViewport(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y));
		_snapshot->execute(context);
	}
}
