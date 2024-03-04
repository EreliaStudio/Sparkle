#pragma once 

#include "game_engine/spk_game_object.hpp"
#include "game_engine/component/spk_camera_component.hpp"

namespace spk
{
	class Camera : public spk::GameObject
	{
	public:
		using Type = CameraComponent::Type;

	private:
		CameraComponent* _cameraComponent;

	public:
		Camera(const std::string& p_name, spk::GameObject* p_parent = nullptr) :
			GameObject(p_name, p_parent),
			_cameraComponent(addComponent<CameraComponent>("Camera"))
		{

		}

		static CameraComponent* mainCamera(){ return (CameraComponent::mainCamera()); }

		void setAsMainCamera()
		{
			_cameraComponent->setAsMainCamera();
		}

		void setType(const CameraComponent::Type& p_type)
		{
			_cameraComponent->setType(p_type);
		}

        const CameraComponent::Type& type() const
		{
			return (_cameraComponent->type());
		}

		void setNearPlane(const float& p_nearPlane)
		{
			_cameraComponent->setNearPlane(p_nearPlane);
		}
		
		const float& nearPlane() const
		{
			return (_cameraComponent->nearPlane());
		}

		void setFarPlane(const float& p_farPlane)
		{
			_cameraComponent->setFarPlane(p_farPlane);
		}

		const float& farPlane() const
		{
			return (_cameraComponent->farPlane());
		}

		void setFOV(const float& p_fov)
		{
			_cameraComponent->setFOV(p_fov);
		}

		const float& fov() const
		{
			return (_cameraComponent->fov());
		}

		void setAspectRatio(const float& p_aspectRatio)
		{
			_cameraComponent->setAspectRatio(p_aspectRatio);
		}

		const float& aspectRatio() const
		{
			return (_cameraComponent->aspectRatio());
		}

		void setOrthographicSize(const spk::Vector2& p_orthographicSize)
		{
			_cameraComponent->setOrthographicSize(p_orthographicSize);
		}

		const spk::Vector2& orthographicSize() const
		{
			return (_cameraComponent->orthographicSize());
		}		
	};
}