#pragma once

#include "math/spk_vector3.hpp"
#include "game_engine/component/spk_game_component.hpp"
#include "design_pattern/spk_observable_value.hpp"

namespace spk
{
	/**
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
	private:
		using ObservableVector3 = spk::ObservableValue<spk::Vector3>;

	public:
		using Contract = spk::ObservableValue<spk::Vector3>::Contract;
		ObservableVector3 translation; 
		ObservableVector3 scale;
		ObservableVector3 rotation;
		std::unique_ptr<Contract> rotationContract;
		
		spk::Vector3 _forward;
		spk::Vector3 _right;
		spk::Vector3 _up;

	private:
		void _computeDirections();

	public:
		Transform();
		Transform(const Transform& p_other);
		Transform& operator = (const Transform& p_other);

		void lookAt(const spk::Vector3& p_target);

		const spk::Vector3& forward() const;
		const spk::Vector3& right() const;

		const spk::Vector3& up() const;
	};
}