#pragma once

#include "game_engine/component/spk_game_component.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include "game_engine/spk_transform.hpp"

namespace spk
{
	/**
	 * @brief Represents a camera component for rendering scenes.
	 * 
	 * This class is responsible for defining a camera component that can be attached to game objects.
	 * It provides various settings such as camera type (Orthographic or Perspective), field of view (FOV),
	 * aspect ratio, near and far planes, and orthographic size.
	 * 
	 * Usage example:
	 * @code
	 * //Create an object to hold the camera.
	 * spk::GameObject gameObject("CameraObject");
	 * 
	 * // Create a camera component
	 * Camera* camera = gameObject.addComponent<spk::Camera>("MainCamera");
	 * 
	 * // Set camera type to Perspective
	 * camera->setType(Camera::Type::Perspective);
	 * 
	 * // Set camera FOV and aspect ratio
	 * camera->setFOV(60.0f);
	 * camera->setAspectRatio(16.0f / 9.0f);
	 * 
	 * // Set near and far planes
	 * camera->setNearPlane(0.1f);
	 * camera->setFarPlane(100.0f);
	 * 
	 * // Set this camera as the main camera for rendering
	 * camera->setAsMainCamera();
	 * 
	 * // The main camera will be automaticaly send to the cameraConstants attributes if included inside a Pipeline.
	 * @endcode
	 * 
	 * @see GameComponent, GameObject, Pipeline, Pipeline::Constant
	 */
	class Camera : public GameComponent
	{
	public:
		enum class Type
		{
			Orthographic,
			Perspective
		};

	private:
		static inline bool _cameraConstantsInitialized = false;
		static inline spk::Pipeline::Constant* _cameraConstants = nullptr;
		static inline spk::Pipeline::Constant::Element* _cameraConstantsMVPElement = nullptr;
		static bool _initializeCameraConstants();

		static inline Camera* _mainCamera = nullptr;

		bool _needGPUDataUpdate = false;

		Type _type = Type::Orthographic;
		
		float _fov = 90;
        float _aspectRatio = 1.0f;

		spk::Vector2 _orthoSize = spk::Vector2(10, 10);
		float _nearPlane = 0.1f;
		float _farPlane = 1000.0f;
		
		std::unique_ptr<spk::Transform::Contract> _translationContract = nullptr;
		std::unique_ptr<spk::Transform::Contract> _rotationContract = nullptr;

		void _updateGPUData();

	public:
		Camera(const std::string& p_name);

		static Camera* mainCamera(){ return (_mainCamera); }
		void setAsMainCamera();

		void setType(const Type& p_type);

		void setNearPlane(const float& p_nearPlane);
		void setFarPlane(const float& p_farPlane);

		void setFOV(const float& p_fov);
		void setAspectRatio(const float& p_aspectRatio);

		void setOrthographicSize(const spk::Vector2& p_orthographicSize);
		

		void onRender();
		void onUpdate();
	};
}