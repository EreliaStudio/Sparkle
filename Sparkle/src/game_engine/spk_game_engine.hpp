#pragma once

#include <functional>
#include <vector>
#include "game_engine/spk_game_object.hpp"
#include "game_engine/module/spk_game_engine_module.hpp"
#include "profiler/spk_time_metric.hpp"

namespace spk
{
	/**
	 * @class GameEngine
	 * @brief Represents the game engine responsible for rendering and updating game objects.
	 *
	 * This class serves as the core of the game engine and is responsible for rendering and updating game objects.
	 * It provides functions for rendering, updating, subscribing, unsubscribing, and querying game objects.
	 *
	 * Usage example:
	 * @code
	 * // Create a game engine instance
	 * spk::GameEngine gameEngine;
	 *
	 * // Subscribe game objects to the engine
	 * spk::GameObject gameObject1("MyObject");
	 * gameEngine.subscribe(&gameObject1);
	 * spk::GameObject gameObject2("SomeObject");
	 * gameEngine.subscribe(&gameObject2);
	 *
	 * // Render and update game objects
	 * gameEngine.render();
	 * gameEngine.update();
	 *
	 * // Unsubscribe game objects from the engine
	 * gameEngine.unsubscribe(&gameObject1);
	 *
	 * // Query game objects by name
	 * spk::GameObject* foundObject = gameEngine.getObject("MyObject");
	 * std::vector<spk::GameObject*> objectList = gameEngine.getObjectList("SomeObject");
	 * @endcode
	 *
	 * @see GameObject
	 */

	class GameEngine
	{
	public:
		/**
		 * @brief Define the lambda type to provide to GameEngine to search for a specific GameObject.
		*/
		using Comparator = std::function<bool(GameObject *)>;

	private:
		std::vector<GameObject *> _subscribedObjects;
		std::vector<GameEngineModule*> _modules;
		TimeMetric& _timeMetric;

		std::vector<std::unique_ptr<Notifier::Contract>> _onComponentAdditionContracts;

	public:
		/**
		 * @brief Default constructor.
		 * 
		 * Initializes a new instance of the GameEngine class. This instance will be responsible for managing
		 * the lifecycle of game objects, including rendering and updating them according to the game loop.
		 */
		GameEngine();

		/**
         * @brief Adds a new module to the game engine.
         * 
         * This template method dynamically creates and adds a new module of the specified type to the game engine.
         * The module is initialized with the provided arguments and is automatically associated with the game engine.
         * This allows for flexible extension of the game engine's capabilities by adding various types of modules, such
         * as physics, rendering, or input handling modules.
         * 
         * @tparam TModuleName The type of the module to be added.
         * @tparam Args The types of the arguments to be passed to the module's constructor.
         * @param p_args The arguments to be passed to the module's constructor.
         * @return TModuleName* A pointer to the newly created module.
         * 
         * Usage example:
         * @code
         * auto* physicsModule = gameEngine.addModule<PhysicsModule>(spk::Vector3(0, -9.81, 0));
         * @endcode
         */
		template <typename TModuleName, typename ... Args>
		TModuleName* addModule(Args&& ... p_args)
		{
			GameEngineModule::_creatingEngine = this;

			TModuleName* result = new TModuleName(std::forward<Args>(p_args)...);
			_modules.push_back(result);

			GameEngineModule::_creatingEngine = nullptr;

			return (result);
		}

		/**
		 * @brief Renders all subscribed game objects.
		 * 
		 * Iterates through all subscribed game objects and calls their render methods. This function should be
		 * called once per frame to draw all game objects to the screen.
		 */
		void render();

		/**
		 * @brief Updates all subscribed game objects.
		 * 
		 * Iterates through all subscribed game objects and calls their update methods. This function should be
		 * called once per frame to update the state of all game objects based on time or user input.
		 */
		void update();

		/**
		 * @brief Subscribes a game object to the engine.
		 * 
		 * Adds a new game object to the list of subscribed objects. Subscribed objects will be rendered and updated
		 * by the game engine.
		 * 
		 * @param p_newObject Pointer to the GameObject to subscribe.
		 */
		void subscribe(GameObject *p_newObject);

		/**
		 * @brief Unsubscribes a game object from the engine.
		 * 
		 * Removes a game object from the list of subscribed objects. The object will no longer be rendered or updated
		 * by the game engine.
		 * 
		 * @param p_newObject Pointer to the GameObject to unsubscribe.
		 */
		void unsubscribe(GameObject *p_newObject);

		/**
         * @brief Retrieves a constant reference to the list of subscribed game objects.
         * 
         * This method provides read-only access to the internal list of game objects that have been subscribed
         * to the game engine. It allows external entities to query and interact with the current set of active
         * game objects without modifying the subscription state.
         * 
         * @return const std::vector<spk::GameObject*>& A constant reference to the vector of subscribed game objects.
         */
		const std::vector<spk::GameObject*>& subscribedGameObjects() const;

		/**
		 * @brief Gets a single game object by a custom comparator.
		 * 
		 * Searches for and returns the first game object that matches the criteria defined by the provided comparator function.
		 * 
		 * @param p_comparator A function object that defines the search criteria.
		 * @return A pointer to the first GameObject matching the criteria, or nullptr if no match is found.
		 */
		GameObject *getObject(const Comparator &p_comparator);

		/**
		 * @brief Gets a list of game objects by a custom comparator.
		 * 
		 * Returns a list of all game objects that match the criteria defined by the provided comparator function.
		 * 
		 * @param p_comparator A function object that defines the search criteria.
		 * @return A vector of GameObject pointers matching the criteria.
		 */
		std::vector<GameObject *> getObjectList(const Comparator &p_comparator);

		/**
		 * @brief Gets a single game object by name.
		 * 
		 * Searches for and returns the first game object with the specified name.
		 * 
		 * @param p_name The name of the GameObject to search for.
		 * @return A pointer to the GameObject with the specified name, or nullptr if no match is found.
		 */
		GameObject *getObject(const std::string &p_name);

		/**
		 * @brief Gets a list of game objects by name.
		 * 
		 * Returns a list of all game objects with the specified name.
		 * 
		 * @param p_name The name of the GameObjects to search for.
		 * @return A vector of GameObject pointers with the specified name.
		 */
		std::vector<GameObject *> getObjectList(const std::string &p_name);
	};
}