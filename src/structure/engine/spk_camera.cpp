#include "structure/engine/spk_camera.hpp"

namespace spk
{
	Camera::Camera() :
		_projectionMatrix(spk::Matrix4x4::identity())
	{
		setPerspective(45.0f, 1.0f, 0.1f, 1000.0f);
	}

	void Camera::setPerspective(float p_fovDegrees, float p_aspectRatio, float p_nearPlane, float p_farPlane)
	{
		_projectionMatrix = spk::Matrix4x4::perspective(p_fovDegrees, p_aspectRatio, p_nearPlane, p_farPlane);
		_inverseProjectionMatrix = _projectionMatrix.inverse();
	}

	void Camera::setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane, float p_farPlane)
	{
		_projectionMatrix = spk::Matrix4x4::ortho(p_left, p_right, p_bottom, p_top, p_nearPlane, p_farPlane);
		_inverseProjectionMatrix = _projectionMatrix.inverse();
	}

	const spk::Matrix4x4 &Camera::projectionMatrix() const
	{
		return (_projectionMatrix);
	}

	const spk::Matrix4x4 &Camera::inverseProjectionMatrix() const
	{
		return (_inverseProjectionMatrix);
	}
}