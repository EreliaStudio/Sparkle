#include "rendering/command/engine_view_pass_command.hpp"

#include <GL/glew.h>

#include <stdexcept>
#include <utility>

#include "core/context/render_context.hpp"

namespace spk
{
	EngineViewPassCommand::EngineViewPassCommand(
		Framebuffer::Handle target, Phase phase, RGB background) :
		_target(std::move(target)),
		_phase(phase),
		_background(background)
	{
		if (!_target)
		{
			throw std::invalid_argument("EngineView pass requires a framebuffer");
		}
	}

	void EngineViewPassCommand::_configureTarget(RenderContext &context) const
	{
		const Window::Surface::RenderTarget target(*context.targetSurface, _target);
		target.activate(context);
		context.activeRenderTarget = target;
		const auto size = target.size();
		glViewport(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y));
		glDisable(GL_SCISSOR_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDisable(GL_FRAMEBUFFER_SRGB);
	}

	void EngineViewPassCommand::_configurePhase() const
	{
		switch (_phase)
		{
		case Phase::Shadow:
			glDepthMask(GL_TRUE);
			break;
		case Phase::Skybox:
		{
			const GLfloat background[]{_background.r, _background.g, _background.b, 1.0f};
			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			glDepthMask(GL_TRUE);
			glClearBufferfv(GL_COLOR, 0, background);
			glClear(GL_DEPTH_BUFFER_BIT);
			break;
		}
		case Phase::Opaque:
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			break;
		case Phase::WBOIT:
		{
			const GLfloat zero[]{0.0f, 0.0f, 0.0f, 0.0f};
			const GLfloat one[]{1.0f, 1.0f, 1.0f, 1.0f};
			glClearBufferfv(GL_COLOR, 0, zero);
			glClearBufferfv(GL_COLOR, 1, one);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glEnablei(GL_BLEND, 0);
			glEnablei(GL_BLEND, 1);
			glBlendFunci(0, GL_ONE, GL_ONE);
			glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
			break;
		}
		case Phase::Composition:
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glDisable(GL_BLEND);
			glDisablei(GL_BLEND, 1);
			glDisable(GL_CULL_FACE);
			glEnable(GL_FRAMEBUFFER_SRGB);
			break;
		}
	}

	void EngineViewPassCommand::execute(RenderContext &context) const
	{
		if (context.targetSurface == nullptr || context.targetSurface->lifeCycle() != Window::LifeCycle::Ready)
		{
			return;
		}
		_configureTarget(context);
		_configurePhase();
	}
}
