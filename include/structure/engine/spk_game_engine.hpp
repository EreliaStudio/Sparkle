#pragma once

#include <vector>

#include "structure/engine/spk_entity.hpp"

namespace spk
{
	class GameEngine
	{
	private:
		GameObject _centralEntity = spk::GameObject(L"Root object");

	public:
		GameEngine();

		void clear();

		spk::SafePointer<spk::GameObject> centralEntity();
		void addEntity(const spk::SafePointer<GameObject>& p_entity);
		void removeEntity(const spk::SafePointer<GameObject>& p_entity);

		spk::SafePointer<GameObject> getEntity(const std::wstring& p_name);
		spk::SafePointer<const GameObject> getEntity(const std::wstring& p_name) const;
		std::vector<spk::SafePointer<GameObject>> getEntities(const std::wstring& p_name);
		std::vector<spk::SafePointer<const GameObject>> getEntities(const std::wstring& p_name) const;

		spk::SafePointer<GameObject> getEntityByTag(const std::wstring& p_name);
		spk::SafePointer<const GameObject> getEntityByTag(const std::wstring& p_name) const;
		std::vector<spk::SafePointer<GameObject>> getEntitiesByTag(const std::wstring& p_name);
		std::vector<spk::SafePointer<const GameObject>> getEntitiesByTag(const std::wstring& p_name) const;

		bool contains(const std::wstring& p_name) const;

		size_t count(const std::wstring& p_name) const;

		void onPaintEvent(spk::PaintEvent& p_event);
		void onUpdateEvent(spk::UpdateEvent& p_event);
		void onKeyboardEvent(spk::KeyboardEvent& p_event);
		void onMouseEvent(spk::MouseEvent& p_event);
		void onControllerEvent(spk::ControllerEvent& p_event);
		void onTimerEvent(spk::TimerEvent& p_event);
	};
}