#include "game_engine/component/spk_camera_component.hpp"
#include "math/spk_matrix4x4.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	spk::Matrix4x4 CameraComponent::_computeViewMatrix()
	{
		spk::Vector3 cameraPos = owner()->globalPosition();
		spk::Vector3 cameraTarget = cameraPos + owner()->transform().forward(); 
		spk::Vector3 upVector = owner()->transform().up();

		spk::Matrix4x4 result = spk::Matrix4x4::lookAt(cameraPos, cameraTarget, upVector);

		return (std::move(result));
	}

	spk::Matrix4x4 CameraComponent::_computeOrthographicProjectionMatrix()
	{
		float right = _orthoSize.x * 0.5f;
		float left = -right;
		float top = _orthoSize.y * 0.5f;
		float bottom = -top;

		spk::Matrix4x4 projectionMatrix = spk::Matrix4x4::ortho(left, right, bottom, top, _nearPlane, _farPlane);
		
		projectionMatrix.data[2][3] = 0;

		return projectionMatrix;
	}
	
	spk::Matrix4x4 CameraComponent::_computePerspectiveProjectionMatrix()
	{
		spk::Matrix4x4 projectionMatrix = spk::Matrix4x4::perspective(_fov, _aspectRatio, _nearPlane, _farPlane);

		return (projectionMatrix);
	}

	void CameraComponent::_updateGPUData()
	{	
		spk::Matrix4x4 view = _computeViewMatrix();
		spk::Matrix4x4 projection = (_type == Type::Orthographic ? _computeOrthographicProjectionMatrix() : _computePerspectiveProjectionMatrix());

		_inverseProjectionMatrix = projection.inverse();

		_viewElement = view;
		_projectionElement = projection;
		_cameraConstants.update();
	}

	CameraComponent::CameraComponent(const std::string& p_name) :
		GameComponent(p_name),
		_cameraConstants(_preloadPipeline.constant("cameraConstants")),
		_viewElement(_cameraConstants["view"]),
		_projectionElement(_cameraConstants["projection"]),
		_translationContract(owner()->transform().translation.subscribe([&](){_updateGPUData();})),
		_rotationContract(owner()->transform().rotation.subscribe([&](){_updateGPUData();}))
	{

	}

	void CameraComponent::setAsMainCamera()
	{
		_mainCamera = this;
		_needGPUDataUpdate = true;
	}

	void CameraComponent::setType(const CameraComponent::Type& p_type)
	{
		_type = p_type;
		_needGPUDataUpdate = true;
	}

	const CameraComponent::Type& CameraComponent::type() const
	{
		return (_type);
	}

	void CameraComponent::setOrthographicSize(const spk::Vector2& p_orthographicSize)
	{
		_orthoSize = p_orthographicSize;
		if (_type == Type::Orthographic)
			_needGPUDataUpdate = true;
	}

	const spk::Vector2& CameraComponent::orthographicSize() const
	{
		return (_orthoSize);
	}
	
	void CameraComponent::setNearPlane(const float& p_nearPlane)
	{
		_nearPlane = p_nearPlane;
		_needGPUDataUpdate = true;
	}

	const float& CameraComponent::nearPlane() const
	{
		return (_nearPlane);
	}
	
	void CameraComponent::setFarPlane(const float& p_farPlane)
	{
		_farPlane = p_farPlane;
		_needGPUDataUpdate = true;
	}

	const float& CameraComponent::farPlane() const
	{
		return (_farPlane);
	}

	void CameraComponent::setFOV(const float& p_fov)
	{
		_fov = p_fov;
		if (_type == Type::Perspective)
			_needGPUDataUpdate = true;
	}

	const float& CameraComponent::fov() const
	{
		return (_fov);
	}
	
	void CameraComponent::setAspectRatio(const float& p_aspectRatio)
	{
		_aspectRatio = p_aspectRatio;
		if (_type == Type::Perspective)
			_needGPUDataUpdate = true;
	}

	const float& CameraComponent::aspectRatio() const
	{
		return (_aspectRatio);
	}

	void CameraComponent::_onRender()
	{
		

		if (_needGPUDataUpdate == true)
		{
			_updateGPUData();
			_needGPUDataUpdate = false;
		}
	}

	void CameraComponent::_onUpdate()
	{

	}
}