#include "structure/engine/spk_game_engine.hpp"

namespace spk
{
	GameEngine::GameEngine()
	{
		
	}

	void GameEngine::clear()
	{
		_centralEntity.clearChildren();
	}

	spk::SafePointer<spk::Entity> GameEngine::centralEntity()
	{
		return (&_centralEntity);
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


	void GameEngine::onPaintEvent(spk::PaintEvent& p_event)
	{
		_centralEntity.onPaintEvent(p_event);
	}
	
	void GameEngine::onUpdateEvent(spk::UpdateEvent& p_event)
	{
		_centralEntity.onUpdateEvent(p_event);
	}
	
	void GameEngine::onKeyboardEvent(spk::KeyboardEvent& p_event)
	{
		_centralEntity.onKeyboardEvent(p_event);
	}
	
	void GameEngine::onMouseEvent(spk::MouseEvent& p_event)
	{
		_centralEntity.onMouseEvent(p_event);
	}

	void GameEngine::onControllerEvent(spk::ControllerEvent& p_event)
	{
		_centralEntity.onControllerEvent(p_event);
	}
	
	void GameEngine::onTimerEvent(spk::TimerEvent& p_event)
	{
		_centralEntity.onTimerEvent(p_event);
	}
}