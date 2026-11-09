#include "rendering/engine/camera_component3d.hpp"

#include <stdexcept>

#include "engine/transform3d.hpp"

namespace spk
{
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

	Matrix4x4 CameraComponent3D::viewMatrix() const
	{
		if (owner() == nullptr)
		{
			throw std::logic_error("CameraComponent3D requires an Entity3D owner");
		}

		const Transform3D &transform = owner()->transform();
		const Vector3 &position = transform.position(ReferenceFrame::World);
		const Quaternion &rotation = transform.rotation(ReferenceFrame::World);
		return Matrix4x4::rotation(rotation.conjugated()) *
			   Matrix4x4::translation(-position.x, -position.y, -position.z);
	}

	Matrix4x4 CameraComponent3D::viewProjectionMatrix() const
	{
		return projectionMatrix() * viewMatrix();
	}
}
