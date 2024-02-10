#pragma once

#include "game_engine/component/spk_game_component.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
	/**
     * @class Physics
     * @brief Component for simulating basic physics properties and behaviors for game objects.
     *
     * The Physics component allows game objects to have physical properties such as mass, velocity, and kinematic state.
     * It supports basic physics operations like applying forces and accelerations. This component is essential for
     * simulating dynamics in a game environment, enabling objects to move and interact in a physically plausible manner.
     *
     * Usage example:
     * @code
     * spk::GameObject gameObject("DynamicObject");
     * auto physicsComponent = gameObject.addComponent<spk::Physics>("PhysicsComponent");
     * physicsComponent->setMass(10.0f);
     * physicsComponent->setVelocity(spk::Vector3(0, 0, 0));
     * physicsComponent->applyForce(spk::Vector3(10, 0, 0));
     * @endcode
	 * 
	 * @see GameObject, IVector3
     */
	class Physics : public GameComponent
	{
	private:
		bool _isKinematic;
		float _mass;
		spk::Vector3 _velocity;

		void _onRender();
		void _onUpdate();

	public:
        /**
         * @brief Constructs a Physics component with a specified name.
         * 
         * @param p_name The name of the Physics component.
         */
        Physics(const std::string& p_name);

        /**
         * @brief Checks if the object is kinematic.
         * 
         * @return bool True if the object is kinematic, false otherwise.
         */
        bool isKinematic();

        /**
         * @brief Sets the kinematic state of the object.
         * 
         * @param p_state The new kinematic state. True makes the object kinematic, false makes it dynamic.
         */
        void setKinematicState(bool p_state);

        /**
         * @brief Sets the mass of the object.
         * 
         * @param p_mass The mass of the object.
         */
        void setMass(const float& p_mass);

        /**
         * @brief Sets the velocity of the object.
         * 
         * @param p_velocity The new velocity vector of the object.
         */
        void setVelocity(const spk::Vector3& p_velocity);

        /**
         * @brief Applies a force to the object.
         * 
         * @param p_force The force vector to be applied to the object.
         */
        void applyForce(const spk::Vector3& p_force);

        /**
         * @brief Applies an acceleration to the object.
         * 
         * @param p_acceleration The acceleration vector to be applied to the object.
         */
        void applyAcceleration(const spk::Vector3& p_acceleration);

        /**
         * @brief Gets the mass of the object.
         * 
         * @return const float& The mass of the object.
         */
        const float& mass() const;

        /**
         * @brief Gets the current velocity of the object.
         * 
         * @return const spk::Vector3& The current velocity vector of the object.
         */
        const spk::Vector3& velocity() const;

        /**
         * @brief Predicts the next velocity of the object based on the current forces applied.
         * 
         * This method calculates the next velocity tick considering the current forces and acceleration applied to the object.
         * It's useful for predicting the object's movement in the next frame.
         * 
         * @return spk::Vector3 The predicted next velocity vector of the object.
         */
        spk::Vector3 nextVelocityTick() const;
	};
}