#include "math/camera_3d.hpp"

#include <cmath>
#include <stdexcept>

namespace spk
{
	void Camera3D::setPosition(Vector3 position)
	{
		if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
		{
			throw std::invalid_argument("Camera3D position must be finite");
		}
		_position = position;
		_view.invalidate();
	}

	void Camera3D::setRotation(Quaternion rotation)
	{
		for (const float value : rotation.data)
		{
			if (!std::isfinite(value))
			{
				throw std::invalid_argument("Camera3D rotation must be finite");
			}
		}
		_rotation = rotation.normalized();
		_view.invalidate();
	}

	void Camera3D::setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
	{
		_projection = Matrix4x4::ortho(left, right, bottom, top, nearPlane, farPlane);
	}

	void Camera3D::setPerspective(float fieldOfView, float aspectRatio, float nearPlane, float farPlane)
	{
		_projection = Matrix4x4::perspective(fieldOfView, aspectRatio, nearPlane, farPlane);
	}

	const Vector3 &Camera3D::position() const noexcept
	{
		return _position;
	}

	Quaternion Camera3D::rotation() const noexcept
	{
		return _rotation;
	}

	const Matrix4x4 &Camera3D::viewMatrix() const
	{
		return _view.get();
	}

	const Matrix4x4 &Camera3D::projectionMatrix() const noexcept
	{
		return _projection;
	}

	Matrix4x4 Camera3D::viewProjectionMatrix() const
	{
		return _projection * viewMatrix();
	}
}
