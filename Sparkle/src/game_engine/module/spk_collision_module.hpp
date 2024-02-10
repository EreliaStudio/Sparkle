#pragma once

#include "game_engine/module/spk_game_engine_module.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
	/**
     * @class CollisionModule
     * @brief A module for managing and processing collisions between game objects within the game engine.
     *
     * CollisionModule extends GameEngineModule to specifically handle collision detection and response
     * among game objects. It uses a broad-phase and narrow-phase collision detection strategy to efficiently
     * identify potential collisions and confirm actual collisions, respectively. The module maintains a list
     * of game objects and checks for collisions between them during the game loop's update phase.
     * 
     * Usage example:
     * @code
     * spk::GameEngine engine;
     * spk::CollisionModule collisionModule;
     * engine.addModule(&collisionModule);
     * @endcode
	 * 
	 * @see GameEngineModule, GameObject
     */
	class CollisionModule : public spk::GameEngineModule
	{
	private:
		struct Hit
		{
			spk::GameObject* a;
			spk::GameObject* b;
		};

		void _treatCollision(const Hit& p_hit);
		bool _testCollisionBetweenObject(spk::GameObject* p_gameObjectA, spk::GameObject* p_gameObjectB);
		void _onUpdate();
		bool _isObjectRelevant(spk::GameObject* p_gameObject);
	
	public:
		/**
         * @brief Constructs a CollisionModule and initializes its internal state.
         * 
         * The constructor sets up the collision module for use within a game engine, preparing it
         * to manage and process collisions among game objects. It ensures the module is properly
         * integrated with the game engine's update loop and object management systems.
         */
		CollisionModule();
	};
}