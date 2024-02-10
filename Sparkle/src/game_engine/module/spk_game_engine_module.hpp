#pragma once

#include <vector>
#include <string>
#include "profiler/spk_time_metric.hpp"

namespace spk
{
	class GameEngine;
	class GameObject;

	/**
	 * @class GameEngineModule
	 * @brief Abstract base class for modules within the game engine, handling game object updates and relevance.
	 *
	 * The GameEngineModule serves as a foundational component within a game engine, responsible for
	 * managing and updating a subset of game objects deemed relevant to its specific functionality.
	 * It enables modular design by allowing derived classes to implement their own update logic and
	 * relevance criteria for game objects. This class integrates closely with the GameEngine,
	 * supporting a structured and efficient game loop.
	 *
	 * Derived modules must implement the _onUpdate and _isObjectRelevant methods to specify their
	 * behavior and interaction with game objects. The GameEngineModule also facilitates the subscription
	 * and unsubscription of game objects, ensuring that only relevant objects are updated.
	 *
	 * Usage example:
	 * @code
	 * class MyGameModule : public spk::GameEngineModule
	 * {
	 * private:
	 * 		void _onUpdate()
	 * 		{
	 * 			for (auto& object : owner()->subscribedGameObjects())
	 * 			{
	 *				//Do something to the object;
	 * 			}
	 * 		}
	 * 		bool _isObjectRelevant(spk::GameObject* p_gameObject)
	 * 		{
	 * 			if (p_gameObject->getComponent<MyCustomComponent>() != nullptr)	
	 * 				return (true);
	 * 			return (false);
	 * 		}
	 * 	public:
	 * 		MyGameModule(const std::string& p_name) :
	 * 			spk::GameEngineModule(p_name)
	 * 		{
	 * 
	 * 		}
	 * };
	 *
	 * spk::GameEngine engine;
	 * MyGameModule module("ExampleModule");
	 * engine.addModule(&module);
	 * @endcode
	 *
	 * @note This class is designed to be extended by specific module implementations and is not intended
	 * to be instantiated directly.
	 * 
	 * @see GameObject, GameEngine
	 */
	class GameEngineModule
	{
		friend class GameEngine;

	private:
		static inline GameEngine *_creatingEngine = nullptr;
		GameEngine *_owner = nullptr;
		std::string _name;
		std::vector<spk::GameObject *> _relevantObjects;

		TimeMetric &_timeMetric;

		virtual void _onUpdate() = 0;
		virtual bool _isObjectRelevant(spk::GameObject *p_gameObject) = 0;

		void _onGameObjectSubscription(spk::GameObject *p_gameObject);
		void _onGameObjectUnsubscription(spk::GameObject *p_gameObject);
		void update();

	public:
		/**
         * @brief Constructs a GameEngineModule with a specified name, associating it with the game engine.
         * @param p_name The name of the module, used for identification and debugging purposes.
         */
        GameEngineModule(const std::string& p_name);

        /**
         * @brief Retrieves the game engine that owns this module.
         * @return GameEngine* A pointer to the game engine instance that owns this module.
         */
        GameEngine* owner();

        /**
         * @brief Retrieves the game engine that owns this module (const version).
         * @return const GameEngine* A const pointer to the game engine instance that owns this module.
         */
        const GameEngine* owner() const;

        /**
         * @brief Provides access to the collection of game objects deemed relevant by this module.
         * @return std::vector<spk::GameObject*>& A reference to the vector of pointers to relevant game objects.
         */
        std::vector<spk::GameObject*>& relevantObjects();

        /**
         * @brief Provides access to the collection of game objects deemed relevant by this module (const version).
         * @return const std::vector<spk::GameObject*>& A const reference to the vector of pointers to relevant game objects.
         */
		const std::vector<spk::GameObject *> &relevantObjects() const;
	};
}