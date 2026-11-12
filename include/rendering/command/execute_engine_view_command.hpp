#pragma once

#include <memory>

#include "graphics/opengl/framebuffer.hpp"
#include "rendering/command/camera_uniform_render_command.hpp"
#include "rendering/engine/engine_view.hpp"
#include "rendering/render_command.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	class ExecuteEngineViewCommand final : public RenderCommand
	{
	private:
		std::shared_ptr<const RenderSnapshot> _snapshot;
		std::shared_ptr<EngineView::Output> _publisher;
		std::shared_ptr<EngineView::Output::Targets> _targets;
		Texture::Handle _finalTexture;
		CameraUniformRenderCommand _camera;

	public:
		ExecuteEngineViewCommand(
			std::shared_ptr<const RenderSnapshot> snapshot,
			std::shared_ptr<EngineView::Output> publisher,
			std::shared_ptr<EngineView::Output::Targets> targets,
			Texture::Handle finalTexture,
			const Camera3D &camera);

		void execute(RenderContext &context) const override;
	};
}
