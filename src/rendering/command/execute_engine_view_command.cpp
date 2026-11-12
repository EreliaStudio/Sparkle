#include "rendering/command/execute_engine_view_command.hpp"

#include <GL/glew.h>

#include <stdexcept>
#include <utility>

#include "core/context/render_context.hpp"
#include "rendering/command/render_to_framebuffer_command.hpp"

namespace spk
{
	ExecuteEngineViewCommand::ExecuteEngineViewCommand(
		std::shared_ptr<const RenderSnapshot> snapshot,
		std::shared_ptr<EngineView::Output> publisher,
		std::shared_ptr<EngineView::Output::Targets> targets,
		Texture::Handle finalTexture,
		const Camera3D &camera) :
		_snapshot(std::move(snapshot)),
		_publisher(std::move(publisher)),
		_targets(std::move(targets)),
		_finalTexture(std::move(finalTexture)),
		_camera(camera)
	{
		if (!_snapshot || !_publisher || !_targets || !_finalTexture)
		{
			throw std::invalid_argument("ExecuteEngineViewCommand requires a captured view and targets");
		}
	}

	void ExecuteEngineViewCommand::execute(RenderContext &context) const
	{
		if (context.targetSurface == nullptr || context.targetSurface->lifeCycle() != Window::LifeCycle::Ready)
		{
			_snapshot->execute(context);
			return;
		}
		_camera.execute(context);
		RenderToFramebufferCommand(_targets->opaque, _snapshot).execute(context);
		if (glGetError() != GL_NO_ERROR)
		{
			throw std::runtime_error("EngineView rendering failed before publishing its final image");
		}
		_publisher->publish(*context.targetSurface, _finalTexture);
	}
}
