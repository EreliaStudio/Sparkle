#include "rendering/engine/camera_component3d.hpp"

#include <stdexcept>

#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "rendering/engine/engine.hpp"

namespace spk
{
	void CameraComponent3D::_refreshViewCache() const
	{
		if (owner() == nullptr)
		{
			throw std::logic_error("CameraComponent3D requires an Entity3D owner");
		}

		const Transform3D &transform = owner()->transform();
		const WorldPose pose{transform.position(ReferenceFrame::World), transform.rotation(ReferenceFrame::World)};
		if (_cachedWorldPose == pose)
		{
			return;
		}

		_cachedWorldPose = pose;
		_view.invalidate();
		_inverseView.invalidate();
	}

	void CameraComponent3D::setProjectionMode(Camera3D::ProjectionMode mode)
	{
		_projection.setProjectionMode(mode);
	}

	void CameraComponent3D::setPerspective(float fieldOfView, float aspectRatio, float nearPlane, float farPlane)
	{
		_projection.setPerspective(fieldOfView, aspectRatio, nearPlane, farPlane);
	}

	void CameraComponent3D::setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
	{
		_projection.setOrthographic(left, right, bottom, top, nearPlane, farPlane);
	}

	Camera3D::ProjectionMode CameraComponent3D::projectionMode() const noexcept
	{
		return _projection.projectionMode();
	}

	const Matrix4x4 &CameraComponent3D::projectionMatrix() const noexcept
	{
		return _projection.projectionMatrix();
	}

	const Matrix4x4 &CameraComponent3D::viewMatrix() const
	{
		_refreshViewCache();
		return _view.get();
	}

	const Matrix4x4 &CameraComponent3D::inverseViewMatrix() const
	{
		_refreshViewCache();
		return _inverseView.get();
	}

	Matrix4x4 CameraComponent3D::viewProjectionMatrix() const
	{
		return projectionMatrix() * viewMatrix();
	}
	EngineView &CameraComponent3D::view(const std::string &name)
	{
		auto *engine = dynamic_cast<Engine *>(context());
		if (engine == nullptr)
			throw spk::Exception("CameraComponent3D is not attached to a graphical Engine");
		EngineView &candidate = engine->view(name);
		if (candidate.camera() != this)
			throw spk::Exception("EngineView [" + name + "] is not attached to this camera");
		return candidate;
	}

	const EngineView &CameraComponent3D::view(const std::string &name) const
	{
		auto *engine = dynamic_cast<const Engine *>(context());
		if (engine == nullptr)
			throw spk::Exception("CameraComponent3D is not attached to a graphical Engine");
		const EngineView &candidate = engine->view(name);
		if (candidate.camera() != this)
			throw spk::Exception("EngineView [" + name + "] is not attached to this camera");
		return candidate;
	}

}
