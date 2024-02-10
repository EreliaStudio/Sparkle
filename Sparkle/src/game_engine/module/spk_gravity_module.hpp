#pragma once

#include "game_engine/module/spk_game_engine_module.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
	/**
     * @class GravityModule
     * @brief A module within the game engine dedicated to applying gravity to game objects.
     *
     * The GravityModule class extends the GameEngineModule to specifically handle the application
     * of gravitational forces to game objects. It allows for the simulation of gravity within the game
     * environment, affecting objects that are marked as relevant by the module. This can include
     * characters, items, or any game entity that should respond to gravity.
     * 
     * The module uses a Vector3 to represent the direction and magnitude of gravity, which can be
     * configured upon module creation. This allows for flexibility in simulating different gravitational
     * environments, such as varying planet gravities or areas within the game world with altered gravity.
     * 
     * Usage example:
     * @code
     * spk::Vector3 earthGravity(0, -9.81, 0);
     * spk::GravityModule gravityModule(earthGravity);
     * gameEngine.addModule(&gravityModule);
     * @endcode
	 * 
	 * @see GameEngineModule, GameObject
     */
	class GravityModule : public spk::GameEngineModule
	{
	private:
		spk::Vector3 _gravityAcceleration;

		void _applyGravityAcceleration(GameObject* p_gameObject);
		void _onUpdate();
		bool _isObjectRelevant(spk::GameObject* p_gameObject);
	
	public:
		/**
         * @brief Constructs a GravityModule with specified gravitational acceleration.
         * @param p_gravityAcceleration Vector representing the direction and magnitude of gravity.
         *
         * This constructor initializes the module with a gravity acceleration vector, preparing it
         * to apply gravitational forces to relevant game objects.
         */
        GravityModule(const spk::Vector3& p_gravityAcceleration);

        /**
         * @brief Sets the gravitational acceleration for the module.
         * @param p_gravityAcceleration New gravity acceleration vector.
         *
         * This method allows dynamic updating of the module's gravitational acceleration, enabling
         * changes to the gravity effect on game objects during runtime.
         */
        void setGravityAcceleration(const spk::Vector3& p_gravityAcceleration);

        /**
         * @brief Retrieves the current gravitational acceleration of the module.
         * @return const spk::Vector3& Reference to the current gravity acceleration vector.
         *
         * This method provides access to the module's current gravitational acceleration, which
         * defines the direction and magnitude of the gravity applied to objects.
         */
        const spk::Vector3& gravityAcceleration() const;
	};
}