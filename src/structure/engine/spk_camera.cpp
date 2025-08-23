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

	spk::Vector3 Camera::convertScreenToCamera(const spk::Vector3 &p_screenPosition) const
	{
		return ((_inverseProjectionMatrix * spk::Vector4(p_screenPosition, 1.0f)).xyz());
	}

	spk::Vector3 Camera::convertScreenToCamera(const spk::Vector2 &p_screenPosition) const
	{
		return (convertScreenToCamera(spk::Vector3(p_screenPosition, -1.0f)));
	}
}