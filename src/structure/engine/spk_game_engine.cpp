#include "structure/engine/spk_game_engine.hpp"

namespace spk
{
	GameEngine::GameEngine()
	{
		_rootObject.activate();
	}

	void GameEngine::clear()
	{
		_rootObject.clearChildren();
	}

	spk::SafePointer<spk::Entity> GameEngine::rootObject()
	{
		return (&_rootObject);
	}

	void GameEngine::addEntity(const spk::SafePointer<Entity> &p_entity)
	{
		_rootObject.addChild(p_entity);
	}

	void GameEngine::removeEntity(const spk::SafePointer<Entity> &p_entity)
	{
		_rootObject.removeChild(p_entity);
	}

	spk::SafePointer<Entity> GameEngine::getEntity(const std::wstring &p_name)
	{
		return (_rootObject.getChild(p_name));
	}

	spk::SafePointer<const Entity> GameEngine::getEntity(const std::wstring &p_name) const
	{
		return (_rootObject.getChild(p_name));
	}

	std::vector<spk::SafePointer<Entity>> GameEngine::getEntities(const std::wstring &p_name)
	{
		return (_rootObject.getChildren(p_name));
	}

	std::vector<spk::SafePointer<const Entity>> GameEngine::getEntities(const std::wstring &p_name) const
	{
		return (_rootObject.getChildren(p_name));
	}

	bool GameEngine::contains(const std::wstring &p_name) const
	{
		return (_rootObject.contains(p_name));
	}

	size_t GameEngine::count(const std::wstring &p_name) const
	{
		return (_rootObject.count(p_name));
	}

	void GameEngine::onGeometryChange(const spk::Geometry2D& p_geometry)
	{
		_rootObject.onGeometryChange(p_geometry);
	}

	void GameEngine::onPaintEvent(spk::PaintEvent &p_event)
	{
		_rootObject.onPaintEvent(p_event);
	}

	void GameEngine::onUpdateEvent(spk::UpdateEvent &p_event)
	{
		_rootObject.onUpdateEvent(p_event);
	}

	void GameEngine::onKeyboardEvent(spk::KeyboardEvent &p_event)
	{
		_rootObject.onKeyboardEvent(p_event);
	}

	void GameEngine::onMouseEvent(spk::MouseEvent &p_event)
	{
		_rootObject.onMouseEvent(p_event);
	}

	void GameEngine::onControllerEvent(spk::ControllerEvent &p_event)
	{
		_rootObject.onControllerEvent(p_event);
	}

	void GameEngine::onTimerEvent(spk::TimerEvent &p_event)
	{
		_rootObject.onTimerEvent(p_event);
	}
}