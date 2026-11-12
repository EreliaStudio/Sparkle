#include "rendering/command/camera_uniform_render_command.hpp"

#include "engine/reference_frame.hpp"
#include "engine/transform3d.hpp"
#include "math/camera_3d.hpp"

namespace spk
{
	CameraUniformRenderCommand::CameraData CameraUniformRenderCommand::_capture(const Camera3D &camera)
	{
		const Matrix4x4 view = camera.viewMatrix();
		return {
			camera.projectionMatrix(),
			view,
			Matrix4x4::translation(camera.position()) * Matrix4x4::rotation(camera.rotation()),
			camera.projectionMatrix() * view};
	}

	CameraUniformRenderCommand::CameraData CameraUniformRenderCommand::_capture(
		const Camera3D &camera, const Transform3D &transform)
	{
		Camera3D positioned = camera;
		positioned.setPosition(transform.position(ReferenceFrame::World));
		positioned.setRotation(transform.rotation(ReferenceFrame::World));
		return _capture(positioned);
	}

	CameraUniformRenderCommand::CameraUniformRenderCommand(CameraData data) :
		_buffer(BindingPoint, sizeof(CameraData))
	{
		_buffer.setData(data);
		_buffer.validate();
	}

	CameraUniformRenderCommand::CameraUniformRenderCommand(const Camera3D &camera) :
		CameraUniformRenderCommand(_capture(camera))
	{
	}

	CameraUniformRenderCommand::CameraUniformRenderCommand(
		const Camera3D &camera, const Transform3D &transform) :
		CameraUniformRenderCommand(_capture(camera, transform))
	{
	}

	void CameraUniformRenderCommand::execute(RenderContext &context) const
	{
		_buffer.activate(context);
	}
}
