#include "rendering/command/engine_view_pass_command.hpp"

#include <stdexcept>
#include <utility>

#include "core/context/render_context.hpp"
#include "graphics/opengl/state.hpp"

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
		OpenGL::State::Viewport{{0, 0, static_cast<int>(size.x), static_cast<int>(size.y)}}.apply();
		OpenGL::State::Scissor{.enabled = false}.apply();
		OpenGL::State::ColorOutput::writable(_phase == Phase::Composition).apply();
		OpenGL::State::StencilTest{.enabled = false}.apply();
	}

	void EngineViewPassCommand::_configurePhase() const
	{
		using State = OpenGL::State;
		State::Rasterization raster;
		State::DepthTest depth;
		auto blend = State::Blend::disabled();
		switch (_phase)
		{
		case Phase::Shadow:
			break;
		case Phase::Skybox:
			raster.cullEnabled = false;
			depth.enabled = false;
			break;
		case Phase::Opaque:
			break;
		case Phase::WBOIT:
			depth.write = false;
			blend.attachments[0] = {.enabled = true, .sourceRGB = State::BlendFactor::One,
				.destinationRGB = State::BlendFactor::One, .sourceAlpha = State::BlendFactor::One,
				.destinationAlpha = State::BlendFactor::One};
			blend.attachments[1] = {.enabled = true, .sourceRGB = State::BlendFactor::Zero,
				.destinationRGB = State::BlendFactor::OneMinusSourceColor,
				.sourceAlpha = State::BlendFactor::Zero,
				.destinationAlpha = State::BlendFactor::OneMinusSourceColor};
			break;
		case Phase::Composition:
			raster.cullEnabled = false;
			depth.enabled = false;
			depth.write = false;
			break;
		}
		raster.apply();
		depth.apply();
		blend.apply();
		if (_phase == Phase::Skybox)
		{
			State::Clear::color(0, {_background.r, _background.g, _background.b, 1.0f});
			State::Clear::depth();
		}
		if (_phase == Phase::WBOIT)
		{
			State::Clear::color(0, {0, 0, 0, 0});
			State::Clear::color(1, {1, 1, 1, 1});
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
