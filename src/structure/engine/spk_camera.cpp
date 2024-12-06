#include "structure/engine/spk_camera.hpp"

#include "structure/engine/spk_entity.hpp"

namespace spk
{
	Camera::Camera(const std::wstring& p_name) :
			Component(p_name),
			_viewMatrix(spk::Matrix4x4::identity()),
			_projectionMatrix(spk::Matrix4x4::identity())
        {
            // Default to a perspective projection
            setPerspective(60.0f, 1.77f, 0.1f, 1000.0f);
        }

        void Camera::setPerspective(float p_fovDegrees, float p_aspectRatio, float p_nearPlane, float p_farPlane)
        {
            _projectionMatrix = spk::Matrix4x4::perspective(p_fovDegrees, p_aspectRatio, p_nearPlane, p_farPlane);
        }

        void Camera::setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane, float p_farPlane)
        {
            _projectionMatrix = spk::Matrix4x4::ortho(p_left, p_right, p_bottom, p_top, p_nearPlane, p_farPlane);
        }

        const spk::Matrix4x4& Camera::viewMatrix()
        {
            _updateViewMatrix();
            return _viewMatrix;
        }

        const spk::Matrix4x4& Camera::projectionMatrix() const
        {
            return _projectionMatrix;
        }

        spk::Matrix4x4 Camera::viewProjectionMatrix()
        {
            _updateViewMatrix();
            return _projectionMatrix * _viewMatrix;
        }

        void Camera::update(const long long& p_duration)
        {
            _updateViewMatrix();
        }

        void Camera::_updateViewMatrix()
        {
            if (owner() == nullptr)
                return;

            const spk::Transform& t = owner()->transform();

            spk::Vector3 cameraPosition = t.position();
            spk::Vector3 cameraForward = t.forward();
            spk::Vector3 cameraUp = t.up();

            spk::Vector3 target = cameraPosition + cameraForward;

            _viewMatrix = spk::Matrix4x4::lookAt(cameraPosition, target, cameraUp);
        }
}