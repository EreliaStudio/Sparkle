#pragma once 

#include "game_engine/spk_game_object.hpp"
#include "game_engine/component/spk_camera_component.hpp"

namespace spk
{
	/**
	 * @class Camera
	 * @brief Represents a camera in the game scene, extending the functionality of GameObject.
	 *
	 * This class serves as a wrapper around CameraComponent, providing a more specialized interface for camera-related actions
	 * within a GameObject context. It simplifies the process of setting up and manipulating camera properties by aggregating
	 * common camera operations.
	 * 
	 * Usage example:
	 * @code
	 * spk::GameEngine engine;
	 * spk::Camera camera("MainCamera");
	 * 
	 * camera.setType(spk::CameraComponent::Type::Perspective);
	 * 
	 * camera.setFOV(45.0f);
	 * camera.setAspectRatio(16.0f / 9.0f);
	 * camera.setNearPlane(0.1f);
	 * camera.setFarPlane(1000.0f);
	 * 
	 * camera.setAsMainCamera();
	 * engine.subscribe(&camera);
	 * @endcode
	 */
	class Camera : public spk::GameObject
	{
	public:
		using Type = CameraComponent::Type; //< Uses the Type enum from CameraComponent for consistency.

	private:
		CameraComponent* _cameraComponent;

	public:
		/**
		 * @brief Constructs a Camera instance with a name and optional parent.
		 * 
		 * Creates a Camera object, initializing it with a CameraComponent and setting its parent GameObject if provided.
		 * 
		 * @param p_name The name of the camera.
		 * @param p_parent Optional parent GameObject.
		 */
		Camera(const std::string& p_name, spk::GameObject* p_parent = nullptr) :
			GameObject(p_name, p_parent),
			_cameraComponent(addComponent<CameraComponent>("Camera"))
		{

		}

		/**
		 * @brief Gets the main camera in the scene.
		 * 
		 * @return A pointer to the main CameraComponent.
		 */
		static CameraComponent* mainCamera(){ return (CameraComponent::mainCamera()); }

		/**
		 * @brief Sets this camera as the main camera for rendering.
		 */
		void setAsMainCamera()
		{
			_cameraComponent->setAsMainCamera();
		}

		/**
		 * @brief Sets the camera's projection type.
		 * 
		 * @param p_type The projection type to set.
		 */
		void setType(const CameraComponent::Type& p_type)
		{
			_cameraComponent->setType(p_type);
		}

		/**
		 * @brief Gets the camera's projection type.
		 * 
		 * @return The current camera projection type.
		 */
        const CameraComponent::Type& type() const
		{
			return (_cameraComponent->type());
		}

		/**
		 * @brief Sets the near clipping plane distance.
		 * 
		 * @param p_nearPlane Distance to the near clipping plane.
		 */
		void setNearPlane(const float& p_nearPlane)
		{
			_cameraComponent->setNearPlane(p_nearPlane);
		}
		
		/**
		 * @brief Gets the near clipping plane distance.
		 * 
		 * @return Distance to the near clipping plane.
		 */
		const float& nearPlane() const
		{
			return (_cameraComponent->nearPlane());
		}

		/**
		 * @brief Sets the far clipping plane distance.
		 * 
		 * @param p_farPlane Distance to the far clipping plane.
		 */
		void setFarPlane(const float& p_farPlane)
		{
			_cameraComponent->setFarPlane(p_farPlane);
		}

		/**
		 * @brief Gets the far clipping plane distance.
		 * 
		 * @return Distance to the far clipping plane.
		 */
		const float& farPlane() const
		{
			return (_cameraComponent->farPlane());
		}

		/**
		 * @brief Sets the field of view (FOV).
		 * 
		 * @param p_fov Field of view in degrees.
		 */
		void setFOV(const float& p_fov)
		{
			_cameraComponent->setFOV(p_fov);
		}

		/**
		 * @brief Gets the field of view (FOV).
		 * 
		 * @return Field of view in degrees.
		 */
		const float& fov() const
		{
			return (_cameraComponent->fov());
		}

		/**
		 * @brief Sets the aspect ratio of the camera.
		 * 
		 * @param p_aspectRatio The aspect ratio to set.
		 */
		void setAspectRatio(const float& p_aspectRatio)
		{
			_cameraComponent->setAspectRatio(p_aspectRatio);
		}

		/**
		 * @brief Gets the aspect ratio of the camera.
		 * 
		 * @return The current aspect ratio.
		 */
		const float& aspectRatio() const
		{
			return (_cameraComponent->aspectRatio());
		}

		/**
		 * @brief Sets the size for orthographic projection.
		 * 
		 * @param p_orthographicSize The size to set for orthographic projection.
		 */
		void setOrthographicSize(const spk::Vector2& p_orthographicSize)
		{
			_cameraComponent->setOrthographicSize(p_orthographicSize);
		}

		/**
		 * @brief Gets the size of the orthographic projection.
		 * 
		 * @return The current size for orthographic projection.
		 */
		const spk::Vector2& orthographicSize() const
		{
			return (_cameraComponent->orthographicSize());
		}		
	};
}