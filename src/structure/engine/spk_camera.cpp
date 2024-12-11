#include "structure/engine/spk_camera.hpp"

#include "structure/engine/spk_entity.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	spk::Pipeline::Constant* Camera::_cameraConstants = nullptr;
	spk::Camera* Camera::_activeCamera = nullptr;

	void Camera::awake()
	{
		_onTransformEditionContract = owner()->transform().addOnEditionCallback([&](){
			if (_activeCamera == this)
			{
				_updateConstants();
			}
		});
		_onTransformEditionContract.trigger();
	}
	
	void Camera::sleep()
	{
		_onTransformEditionContract.resign();
	}

	Camera::Camera(const std::wstring& p_name) :
		Component(p_name),
		_projectionMatrix(spk::Matrix4x4::identity())
	{
		setPerspective(45.0f, 1.0f, 0.1f, 1000.0f);
	}

	void Camera::setPerspective(float p_fovDegrees, float p_aspectRatio, float p_nearPlane, float p_farPlane)
	{
		_projectionMatrix = spk::Matrix4x4::perspective(p_fovDegrees, p_aspectRatio, p_nearPlane, p_farPlane);
		_updateConstants();
	}

	void Camera::setOrthographic(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane, float p_farPlane)
	{
		_projectionMatrix = spk::Matrix4x4::ortho(p_left, p_right, p_bottom, p_top, p_nearPlane, p_farPlane);
		_updateConstants();
	}

	const spk::Matrix4x4& Camera::projectionMatrix() const
	{
		return _projectionMatrix;
	}

	void Camera::_updateConstants()
	{
		if (_cameraConstants == nullptr)
		{
			_cameraConstants = &(spk::Pipeline::constants(L"spk::CameraConstants"));
		}

		if (_cameraConstants == nullptr)
			return ;

		if (owner() != nullptr)
		{
			(*_cameraConstants)[L"view"] = owner()->transform().model();
		}
		else
		{
			(*_cameraConstants)[L"view"] = spk::Matrix4x4::identity();
		}
		(*_cameraConstants)[L"projection"] = projectionMatrix();

		_cameraConstants->validate();
	}
	
	void Camera::activate()
	{

		if (_activeCamera != this)
		{
			_activeCamera = this;
			_updateConstants();
		}
	}
}