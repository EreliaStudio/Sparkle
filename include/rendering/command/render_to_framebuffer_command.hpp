#pragma once

#include <memory>

#include "core/window_surface_render_target.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class RenderSnapshot;

	class RenderToFramebufferCommand final : public RenderCommand
	{
	private:
		Framebuffer::Handle _framebuffer;
		std::shared_ptr<const RenderSnapshot> _snapshot;

	public:
		RenderToFramebufferCommand(
			Framebuffer::Handle framebuffer,
			std::shared_ptr<const RenderSnapshot> snapshot);

		void execute(RenderContext &context) const override;
	};
}
