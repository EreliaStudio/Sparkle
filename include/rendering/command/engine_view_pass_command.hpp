#pragma once

#include <cstdint>

#include "graphics/color.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class EngineViewPassCommand final : public RenderCommand
	{
	public:
		enum class Phase : std::uint8_t
		{
			Shadow,
			Skybox,
			Opaque,
			WBOIT,
			Composition
		};

	private:
		Framebuffer::Handle _target;
		Phase _phase;
		RGB _background;

		void _configureTarget(RenderContext &context) const;
		void _configurePhase(RenderContext &context) const;

	public:
		EngineViewPassCommand(Framebuffer::Handle target, Phase phase, RGB background);
		void execute(RenderContext &context) const override;
	};
}
