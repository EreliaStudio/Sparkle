#pragma once

#include <functional>
#include <vector>
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	/**
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
		using Comparator = std::function<bool(GameObject *)>;

	private:
		std::vector<GameObject *> _subscribedObjects;

	public:
		GameEngine();

		void render();
		void update();

		void subscribe(GameObject *p_newObject);
		void unsubscribe(GameObject *p_newObject);

		GameObject *getObject(const Comparator &p_comparator);
		std::vector<GameObject *> getObjectList(const Comparator &p_comparator);

		GameObject *getObject(const std::string &p_name);
		std::vector<GameObject *> getObjectList(const std::string &p_name);
	};
}