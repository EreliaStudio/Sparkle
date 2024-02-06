#include "game_engine/component/spk_camera_component.hpp"
#include "math/spk_matrix4x4.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	bool Camera::_initializeCameraConstants()
	{
		if (_cameraConstants == nullptr)
		{
			_cameraConstants = spk::Pipeline::globalConstant("mainCamera");
		}

		if (_cameraConstants != nullptr)
		{
			if (_cameraConstantsMVPElement == nullptr)
			{
				_cameraConstantsMVPElement = &(_cameraConstants->operator[]("MVP"));
			}
		}

		if (_cameraConstants != nullptr && _cameraConstantsMVPElement != nullptr)
			_cameraConstantsInitialized = true;

		return (_cameraConstantsInitialized);
	}
	
	void Camera::_updateGPUData()
	{		
		spk::Matrix4x4 projection;
		
		if (_type == Type::Orthographic)
			projection = spk::Matrix4x4::ortho(-_orthoSize.x / 2.0f, _orthoSize.x / 2.0f, _orthoSize.y / 2.0f, -_orthoSize.y / 2.0f, _nearPlane, _farPlane);
		else
			projection = spk::Matrix4x4::perspective( _fov, _aspectRatio, _nearPlane, _farPlane );

		spk::Matrix4x4 view = spk::Matrix4x4::lookAt(
			owner()->globalPosition(),
			owner()->globalPosition() + owner()->transform().forward(),
			spk::Vector3(0, 1, 0));

		*_cameraConstantsMVPElement = projection * view;
		_cameraConstants->update();
	}

	Camera::Camera(const std::string& p_name) :
		GameComponent(p_name),
		_translationContract(owner()->transform().translation.subscribe([&](){_needGPUDataUpdate = true;})),
		_rotationContract(owner()->transform().translation.subscribe([&](){_needGPUDataUpdate = true;}))
	{

	}

	void Camera::setAsMainCamera()
	{
		_mainCamera = this;
		_needGPUDataUpdate = true;
	}

	void Camera::setType(const Camera::Type& p_type)
	{
		_type = p_type;
		_needGPUDataUpdate = true;
	}

	void Camera::setOrthographicSize(const spk::Vector2& p_orthographicSize)
	{
		_orthoSize = p_orthographicSize;
		if (_type == Type::Orthographic)
			_needGPUDataUpdate = true;
	}
	
	void Camera::setNearPlane(const float& p_nearPlane)
	{
		_nearPlane = p_nearPlane;
		_needGPUDataUpdate = true;
	}
	
	void Camera::setFarPlane(const float& p_farPlane)
	{
		_farPlane = p_farPlane;
		_needGPUDataUpdate = true;
	}

	void Camera::setFOV(const float& p_fov)
	{
		_fov = p_fov;
		if (_type == Type::Perspective)
			_needGPUDataUpdate = true;
	}
	
	void Camera::setAspectRatio(const float& p_aspectRatio)
	{
		_aspectRatio = p_aspectRatio;
		if (_type == Type::Perspective)
			_needGPUDataUpdate = true;
	}

	void Camera::onRender()
	{
		if (_cameraConstantsInitialized == false)
		{
			if (_initializeCameraConstants() == false)
				return ;
		}

		if (_needGPUDataUpdate == true)
		{
			_updateGPUData();
			_needGPUDataUpdate = false;
		}
	}

	void Camera::onUpdate()
	{

	}
}