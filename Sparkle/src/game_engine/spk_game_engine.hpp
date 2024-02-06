#pragma once

#include <functional>
#include <vector>
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	class GameEngine
	{
	public:
		using Comparator = std::function<bool (GameObject*)>;

	private:
		std::vector<GameObject*> _subscribedObjects;

	public:
		GameEngine();

		void render();
		void update();

		void subscribe(GameObject* p_newObject);
		void unsubscribe(GameObject* p_newObject);

		GameObject* getObject(const Comparator& p_comparator);
		std::vector<GameObject*> getObjectList(const Comparator& p_comparator);
		
		GameObject* getObject(const std::string& p_name);
		std::vector<GameObject*> getObjectList(const std::string& p_name);
	};
}