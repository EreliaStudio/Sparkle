#include "game_engine/component/spk_camera_component.hpp"
#include "math/spk_matrix4x4.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	bool Camera::_initializeCameraConstants()
	{
		if (_cameraConstants == nullptr)
		{
			_cameraConstants = spk::Pipeline::globalConstant("cameraConstants");
		}

		if (_cameraConstants != nullptr)
		{
			if (_cameraConstantsMVPElement == nullptr)
			{
				_cameraConstantsMVPElement = &(_cameraConstants->operator[]("MVP"));
			}
		}

		if (_cameraConstants != nullptr && _cameraConstantsMVPElement != nullptr)
		{
			_cameraConstantsInitialized = true;
		}

		return (_cameraConstantsInitialized);
	}
	
	void Camera::_updateGPUData()
	{		
		spk::Matrix4x4 projection;
		
		if (_type == Type::Orthographic)
			projection = spk::Matrix4x4::ortho(-_orthoSize.x / 2.0f, _orthoSize.x / 2.0f, -_orthoSize.y / 2.0f, _orthoSize.y / 2.0f, _nearPlane, _farPlane);
		else
			projection = spk::Matrix4x4::perspective( _fov, _aspectRatio, _nearPlane, _farPlane );

		projection.data[3][2] = 0;

		spk::Matrix4x4 view = spk::Matrix4x4();//spk::Matrix4x4::lookAt(
			// owner()->globalPosition(),
			// owner()->globalPosition() + owner()->transform().forward(),
			// owner()->transform().up());

		spk::Matrix4x4 model = spk::Matrix4x4();//spk::Matrix4x4::translationMatrix(owner()->globalPosition());

		spk::Matrix4x4 mvp = projection * view * model;

		std::cout << "Camera MVP : " << mvp << std::endl;

		spk::Vector3 points[4] = {
			spk::Vector3(0, 0, 1),
			spk::Vector3(2, 2, 10),
			spk::Vector3(2, 2, 100),
			spk::Vector3(2, 2, 1000)
		};

		for (size_t i = 0; i < 4; i++)
		{
			std::cout << "Point [" << points[i] << "] = " << (mvp * points[i]) << std::endl;
		}

		*_cameraConstantsMVPElement = projection * view * model;
		_cameraConstants->update();
	}

	Camera::Camera(const std::string& p_name) :
		GameComponent(p_name),
		_translationContract(owner()->transform().translation.subscribe([&](){_needGPUDataUpdate = true;})),
		_rotationContract(owner()->transform().rotation.subscribe([&](){_needGPUDataUpdate = true;}))
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

	const Camera::Type& Camera::type() const
	{
		return (_type);
	}

	void Camera::setOrthographicSize(const spk::Vector2& p_orthographicSize)
	{
		_orthoSize = p_orthographicSize;
		if (_type == Type::Orthographic)
			_needGPUDataUpdate = true;
	}

	const spk::Vector2& Camera::orthographicSize() const
	{
		return (_orthoSize);
	}
	
	void Camera::setNearPlane(const float& p_nearPlane)
	{
		_nearPlane = p_nearPlane;
		_needGPUDataUpdate = true;
	}

	const float& Camera::nearPlane() const
	{
		return (_nearPlane);
	}
	
	void Camera::setFarPlane(const float& p_farPlane)
	{
		_farPlane = p_farPlane;
		_needGPUDataUpdate = true;
	}

	const float& Camera::farPlane() const
	{
		return (_farPlane);
	}

	void Camera::setFOV(const float& p_fov)
	{
		_fov = p_fov;
		if (_type == Type::Perspective)
			_needGPUDataUpdate = true;
	}

	const float& Camera::fov() const
	{
		return (_fov);
	}
	
	void Camera::setAspectRatio(const float& p_aspectRatio)
	{
		_aspectRatio = p_aspectRatio;
		if (_type == Type::Perspective)
			_needGPUDataUpdate = true;
	}

	const float& Camera::aspectRatio() const
	{
		return (_aspectRatio);
	}

	void Camera::_onRender()
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

	void Camera::_onUpdate()
	{

	}
}