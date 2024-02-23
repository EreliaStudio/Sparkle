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
			if (_viewElement == nullptr)
			{
				_viewElement = &(_cameraConstants->operator[]("view"));
			}

			if (_projectionElement == nullptr)
			{
				_projectionElement = &(_cameraConstants->operator[]("projection"));
			}
		}

		if (_cameraConstants != nullptr && _viewElement != nullptr && _projectionElement != nullptr)
		{
			_cameraConstantsInitialized = true;
		}

		return (_cameraConstantsInitialized);
	}

	spk::Matrix4x4 Camera::_computeViewMatrix()
	{
		spk::Vector3 cameraPos = owner()->globalPosition();
		spk::Vector3 cameraTarget = cameraPos + owner()->transform().forward(); 
		spk::Vector3 upVector = owner()->transform().up();

		spk::Matrix4x4 result = spk::Matrix4x4::translationMatrix(owner()->globalPosition().inverse());//::lookAt(cameraPos, cameraTarget, upVector);

		return (std::move(result));
	}

	spk::Matrix4x4 Camera::_computeOrthographicProjectionMatrix()
	{
		float right = _orthoSize.x * 0.5f;
		float left = -right;
		float top = _orthoSize.y * 0.5f;
		float bottom = -top;

		spk::Matrix4x4 projectionMatrix = spk::Matrix4x4::ortho(left, right, bottom, top, _nearPlane, _farPlane);
		
		projectionMatrix.data[2][3] = 0;

		return projectionMatrix;
	}
	
	spk::Matrix4x4 Camera::_computePerspectiveProjectionMatrix()
	{
		return (spk::Matrix4x4::perspective(_fov, _aspectRatio, _nearPlane, _farPlane));
	}

	void Camera::_updateGPUData()
	{		
		spk::Matrix4x4 view = _computeViewMatrix();
		spk::Matrix4x4 projection = (_type == Type::Orthographic ? _computeOrthographicProjectionMatrix() : _computePerspectiveProjectionMatrix());

		*_viewElement = view;
		*_projectionElement = projection;
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