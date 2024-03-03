#pragma once

#include "game_engine/component/spk_game_component.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include "game_engine/spk_transform.hpp"
#include "math/spk_matrix4x4.hpp"

namespace spk
{
	/**
	 * @class Camera
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
		/**
		 * @brief Enum class for camera type.
		 * 
		 * Specifies the camera projection type. Orthographic projection is useful for 2D games,
		 * while Perspective projection is used for 3D scene rendering to simulate depth perception.
		 */
		enum class Type
		{
			Orthographic,
			Perspective
		};

	private:
		static inline const std::string _preloadPipelineCode = R"(#version 450

		#include <cameraConstants>

		void geometryPass()
		{

		}

		void renderPass()
		{

		})";
		static inline spk::Pipeline _preloadPipeline = spk::Pipeline(_preloadPipelineCode);

		spk::Pipeline::Constant& _cameraConstants;
		spk::Pipeline::Constant::Element& _viewElement;
		spk::Pipeline::Constant::Element& _projectionElement;

		static inline Camera* _mainCamera = nullptr;

		bool _needGPUDataUpdate = false;

		Type _type = Type::Orthographic;
		
		spk::Matrix4x4 _inverseProjectionMatrix;

		float _fov = 90;
        float _aspectRatio = 1.0f;

		spk::Vector2 _orthoSize = spk::Vector2(10, 10);
		float _nearPlane = 0.0f;
		float _farPlane = 1000.0f;
		
		std::unique_ptr<spk::Transform::Contract> _translationContract = nullptr;
		std::unique_ptr<spk::Transform::Contract> _rotationContract = nullptr;

		spk::Matrix4x4 _computeViewMatrix();
		spk::Matrix4x4 _computeOrthographicProjectionMatrix();
		spk::Matrix4x4 _computePerspectiveProjectionMatrix();
		void _updateGPUData();
		
		void _onRender();
		void _onUpdate();

	public:
		/**
		 * @brief Constructs a Camera component with a specified name.
		 * 
		 * Initializes a new Camera component, setting it as activated by default and initializing it with default
		 * projection settings.
		 * 
		 * @param p_name The name of the camera component, used for identification within the game engine.
		 */
		Camera(const std::string& p_name);

		/**
		 * @brief Sets this camera as the main camera for rendering.
		 * 
		 * Marks this camera instance as the main camera, which will be used by the rendering system to render scenes.
		 * Only one camera can be the main camera at a time.
		 */
		static Camera* mainCamera(){ return (_mainCamera); }

		/**
		 * @brief Gets the current main camera.
		 * 
		 * @return A pointer to the Camera instance set as the main camera, or nullptr if no main camera has been set.
		 */
		void setAsMainCamera();

		/**
		 * @brief Sets the camera projection type.
		 * 
		 * @param p_type The camera type (Orthographic or Perspective).
		 */
		void setType(const Type& p_type);

		/**
         * @brief Gets the current camera projection type.
         * 
         * This method returns the current projection type of the camera, which can be either Orthographic or Perspective.
         * The projection type affects how scenes are rendered through this camera.
         * 
         * @return The current Type of the camera's projection.
         */
        const Type& type() const;

		/**
		 * @brief Sets the near clipping plane distance.
		 * 
		 * @param p_nearPlane The distance to the near clipping plane. Objects closer than this distance will not be rendered.
		 */
		void setNearPlane(const float& p_nearPlane);
		
        /**
         * @brief Gets the distance to the near clipping plane.
         * 
         * The near clipping plane is the minimum distance at which the camera starts rendering scenes. Objects closer than this distance will not be visible.
         * 
         * @return The distance to the near clipping plane.
         */
		const float& nearPlane() const;

		/**
		 * @brief Sets the far clipping plane distance.
		 * 
		 * @param p_farPlane The distance to the far clipping plane. Objects farther than this distance will not be rendered.
		 */
		void setFarPlane(const float& p_farPlane);

		/**
         * @brief Gets the distance to the far clipping plane.
         * 
         * The far clipping plane is the maximum distance at which the camera can render scenes. Objects beyond this distance will not be visible.
         * 
         * @return The distance to the far clipping plane.
         */
		const float& farPlane() const;

		/**
		 * @brief Sets the field of view (FOV) for the camera.
		 * 
		 * Applicable only in Perspective mode. Defines the vertical field of view angle.
		 * 
		 * @param p_fov The field of view angle in degrees.
		 */
		void setFOV(const float& p_fov);

		/**
         * @brief Gets the field of view (FOV) of the camera.
         * 
         * Applicable only when the camera is set to Perspective mode. The FOV is the vertical angle of the camera's view cone, affecting how wide the scene appears.
         * 
         * @return The field of view angle in degrees.
         */
		const float& fov() const;

		/**
		 * @brief Sets the aspect ratio of the camera.
		 * 
		 * Defines the ratio of the camera's viewport width to its height.
		 * 
		 * @param p_aspectRatio The aspect ratio value.
		 */
		void setAspectRatio(const float& p_aspectRatio);

		/**
         * @brief Gets the aspect ratio of the camera.
         * 
         * The aspect ratio is defined as the ratio of the camera's viewport width to its height. It affects the shape of the rendered scene.
         * 
         * @return The aspect ratio of the camera.
         */
		const float& aspectRatio() const;

		/**
		 * @brief Sets the size for orthographic projection.
		 * 
		 * Applicable only in Orthographic mode. Defines the size of the viewable area.
		 * 
		 * @param p_orthographicSize The size of the orthographic projection area.
		 */
		void setOrthographicSize(const spk::Vector2& p_orthographicSize);

		/**
         * @brief Gets the size of the orthographic projection.
         * 
         * Applicable only when the camera is set to Orthographic mode. This defines the size of the viewable area, controlling how much of the scene is visible.
         * 
         * @return The size of the orthographic projection area as a spk::Vector2.
         */
		const spk::Vector2& orthographicSize() const;
};
}