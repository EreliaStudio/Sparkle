#include "rendering/command/camera_uniform_render_command.hpp"

namespace spk
{
	CameraUniformRenderCommand::CameraUniformRenderCommand(CameraData data) :
		_buffer(BindingPoint, sizeof(CameraData))
	{
		_buffer.setData(data);
		_buffer.validate();
	}

	void CameraUniformRenderCommand::execute(RenderContext &context) const
	{
		_buffer.activate(context);
	}
}
