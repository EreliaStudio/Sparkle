#include "structure/engine/spk_game_engine.hpp"

namespace spk
{
	GameEngine::GameEngine()
	{
		
	}

	void GameEngine::addEntity(const spk::SafePointer<Entity>& p_entity)
	{
		_centralEntity.addChild(p_entity);
	}

	void GameEngine::removeEntity(const spk::SafePointer<Entity>& p_entity)
	{
		_centralEntity.removeChild(p_entity);
	}

	spk::SafePointer<Entity> GameEngine::getEntity(const std::wstring& p_name)
	{
		return _centralEntity.getChild(p_name);
	}

	spk::SafePointer<const Entity> GameEngine::getEntity(const std::wstring& p_name) const
	{
		return _centralEntity.getChild(p_name);
	}

	std::vector<spk::SafePointer<Entity>> GameEngine::getEntities(const std::wstring& p_name)
	{
		return (_centralEntity.getChildren(p_name));
	}

	std::vector<spk::SafePointer<const Entity>> GameEngine::getEntities(const std::wstring& p_name) const
	{
		return (_centralEntity.getChildren(p_name));
	}

	bool GameEngine::contains(const std::wstring& p_name) const
	{
		return (_centralEntity.contains(p_name));
	}

	size_t GameEngine::count(const std::wstring& p_name) const
	{
		return (_centralEntity.count(p_name));
	}

	void GameEngine::update(const long long& p_deltaTime)
	{
		_centralEntity.update(p_deltaTime);
	}

	void GameEngine::render()
	{
		_centralEntity.render();
	}
}