#pragma once

#include "math/spk_vector3.hpp"
#include "math/spk_quaternion.hpp"
#include "game_engine/component/spk_game_component.hpp"
#include "design_pattern/spk_observable_value.hpp"

namespace spk
{
	/**
	 * @class Transform
	 * @brief Represents the transformation of a game object in 3D space.
	 *
	 * This class defines the position, rotation, and scale of a game object within a 3D world.
	 * It provides observable values for each transformation component, allowing changes to be monitored
	 * and reacted to by other parts of the game engine.
	 * This class is automaticaly create by composition inside a game object.
	 *
	 * Usage example:
	 * @code
	 * GameObject gameObject("MyObject");
	 * Transform& transform = gameObject.transform();
	 *
	 * // Subscribe to the translation property changes
	 * auto translationContract = transform.translation.subscribe([]() {
	 *     // This code will be executed whenever the translation property changes
	 *     std::cout << "Translation changed to: (" << transform.translation.get() << ")" << std::endl;
	 * });
	 *
	 * // Change the translation, which triggers the subscribed callback
	 * transform.translation = spk::Vector3(1.0f, 2.0f, 3.0f);
	 * @endcode
	 * 
	 * @see ObservableValue, IVector3, GameObject
	 */
	class Transform
	{
		friend class GameObject;
	private:
		using ObservableVector3 = spk::ObservableValue<spk::Vector3>;
		using ObservableQuaternion = spk::ObservableValue<spk::Quaternion>;

	public:
		/**
		 * @brief The type of contract provided by translation and scale uppon subscription.
		*/
		using Contract = ObservableVector3::Contract;
		/**
		 * @brief The type of contract provided by rotation uppon subscription.
		*/
		using RotationContract = ObservableQuaternion::Contract;

		/**
		 * @brief Observable translation vector.
		 * 
		 * Represents the position of the Transform in 3D space. Changes to this property can be observed by subscribing
		 * to it, allowing for reactive behaviors in response to position changes.
		 */
		ObservableVector3 translation; 

		/**
		 * @brief Observable scale vector.
		 * 
		 * Represents the scale of the Transform in 3D space. Changes to this property can be observed by subscribing
		 * to it, enabling dynamic responses to scale adjustments.
		 */
		ObservableVector3 scale;

		/**
		 * @brief Observable rotation vector.
		 * 
		 * Represents the rotation of the Transform in 3D space, using Euler angles. Subscribing to this property allows
		 * for reactions to rotation changes.
		 */
		ObservableQuaternion rotation;

	private:
		GameObject *_owner = nullptr;
		std::unique_ptr<Contract> rotationContract;
		spk::Vector3 _forward;
		spk::Vector3 _right;
		spk::Vector3 _up;
		void _computeDirections();

		void _bind(GameObject* p_owner);

	public:
		/**
		 * @brief Default constructor.
		 * 
		 * Initializes a new Transform instance with default values for translation, scale, and rotation.
		 * The default orientation vectors (forward, right, up) are also initialized based on the identity rotation.
		 */
		Transform();

		/**
		 * @brief Copy constructor.
		 * 
		 * Creates a new Transform instance as a copy of an existing Transform. This includes copying the translation,
		 * scale, rotation values, and the orientation vectors.
		 * 
		 * @param p_other The Transform instance to copy from.
		 */
		Transform(const Transform& p_other);

		/**
		 * @brief Copy assignment operator.
		 * 
		 * Copies the translation, scale, rotation values, and the orientation vectors from another Transform instance to this one.
		 * 
		 * @param p_other The Transform instance to copy from.
		 * @return A reference to this Transform instance after copying the data.
		 */
		Transform& operator=(const Transform& p_other);

		/**
		 * @brief Sets the object's orientation to look at a specific target point in space.
		 * 
		 * Adjusts the object's rotation so that its forward vector points directly at the specified target point in space.
		 * This method automatically updates the orientation vectors (forward, right, up) to reflect the new rotation.
		 * 
		 * @param p_target The target point in space to look at.
		 */
		void lookAt(const spk::Vector3& p_target);

		/**
		 * @brief Gets the forward orientation vector.
		 * 
		 * Returns the vector representing the forward direction of the Transform, based on its current rotation.
		 * 
		 * @return A const reference to the forward vector.
		 */
		const spk::Vector3& forward() const;

		/**
		 * @brief Gets the right orientation vector.
		 * 
		 * Returns the vector representing the right direction of the Transform, based on its current rotation.
		 * 
		 * @return A const reference to the right vector.
		 */
		const spk::Vector3& right() const;

		/**
		 * @brief Gets the up orientation vector.
		 * 
		 * Returns the vector representing the up direction of the Transform, based on its current rotation.
		 * 
		 * @return A const reference to the up vector.
		 */
		const spk::Vector3& up() const;
	};
}