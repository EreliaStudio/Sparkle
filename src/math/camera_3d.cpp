#include "math/camera_3d.hpp"

#include <cmath>
#include <stdexcept>

namespace spk
{
	Camera3D::Camera3D(const Camera3D &other) :
		_position(other._position),
		_rotation(other._rotation),
		_projectionMode(other._projectionMode),
		_perspective(other._perspective),
		_orthographic(other._orthographic)
	{
	}

	Camera3D::Camera3D(Camera3D &&other) :
		Camera3D(static_cast<const Camera3D &>(other))
	{
	}

	Camera3D &Camera3D::operator=(const Camera3D &other)
	{
		if (this != &other)
		{
			_position = other._position;
			_rotation = other._rotation;
			_projectionMode = other._projectionMode;
			_perspective = other._perspective;
			_orthographic = other._orthographic;
			_view.invalidate();
			_viewProjection.invalidate();
		}
		return *this;
	}

	Camera3D &Camera3D::operator=(Camera3D &&other)
	{
		return operator=(static_cast<const Camera3D &>(other));
	}

	void Camera3D::setPosition(Vector3 position)
	{
		if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
		{
			throw std::invalid_argument("Camera3D position must be finite");
		}
		if (_position == position)
		{
			return;
		}
		_position = position;
		_view.invalidate();
		_viewProjection.invalidate();
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
		const Quaternion normalized = rotation.normalized();
		if (_rotation == normalized)
		{
			return;
		}
		_rotation = normalized;
		_view.invalidate();
		_viewProjection.invalidate();
	}

	void Camera3D::lookAt(Vector3 target, Vector3 up)
	{
		if (!std::isfinite(target.x) || !std::isfinite(target.y) || !std::isfinite(target.z) ||
			!std::isfinite(up.x) || !std::isfinite(up.y) || !std::isfinite(up.z))
		{
			throw std::invalid_argument("Camera3D lookAt vectors must be finite");
		}
		setRotation(Quaternion::lookAt(_position, target, up));
	}

	void Camera3D::setProjectionMode(ProjectionMode mode)
	{
		if (mode != ProjectionMode::Perspective && mode != ProjectionMode::Orthographic)
		{
			throw std::invalid_argument("Invalid Camera3D projection mode");
		}
		if (_projectionMode == mode)
		{
			return;
		}
		_projectionMode = mode;
		_viewProjection.invalidate();
	}

	void Camera3D::setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
	{
		if (!std::isfinite(left) || !std::isfinite(right) || !std::isfinite(bottom) ||
			!std::isfinite(top) || !std::isfinite(nearPlane) || !std::isfinite(farPlane) ||
			right <= left || top <= bottom || farPlane <= nearPlane)
		{
			throw std::invalid_argument("Invalid Camera3D orthographic bounds");
		}
		_orthographic = Matrix4x4::ortho(left, right, bottom, top, nearPlane, farPlane);
		_projectionMode = ProjectionMode::Orthographic;
		_viewProjection.invalidate();
	}

	void Camera3D::setPerspective(float fieldOfView, float aspectRatio, float nearPlane, float farPlane)
	{
		_perspective = Matrix4x4::perspective(fieldOfView, aspectRatio, nearPlane, farPlane);
		_projectionMode = ProjectionMode::Perspective;
		_viewProjection.invalidate();
	}

	const Vector3 &Camera3D::position() const noexcept
	{
		return _position;
	}

	Quaternion Camera3D::rotation() const noexcept
	{
		return _rotation;
	}

	Camera3D::ProjectionMode Camera3D::projectionMode() const noexcept
	{
		return _projectionMode;
	}

	const Matrix4x4 &Camera3D::viewMatrix() const
	{
		return _view.get();
	}

	const Matrix4x4 &Camera3D::projectionMatrix() const noexcept
	{
		return _projectionMode == ProjectionMode::Perspective ? _perspective : _orthographic;
	}

	Matrix4x4 Camera3D::viewProjectionMatrix() const
	{
		return _viewProjection.get();
	}
}
