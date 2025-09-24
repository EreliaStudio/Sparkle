#pragma once

#include <mutex>
#include <set>
#include <vector>

#include "structure/engine/spk_generic_entity.hpp"

namespace spk
{
	class GameEngine
	{
	private:
		GenericEntity _rootObject = spk::GenericEntity(L"/root");
		std::mutex _collisionMutex;

	public:
		GameEngine();

		void clear();

		spk::SafePointer<spk::GenericEntity> rootObject();
		void addEntity(const spk::SafePointer<GenericEntity> &p_entity);
		void removeEntity(const spk::SafePointer<GenericEntity> &p_entity);

		spk::SafePointer<GenericEntity> getEntity(const std::wstring &p_name);
		spk::SafePointer<const GenericEntity> getEntity(const std::wstring &p_name) const;
		std::vector<spk::SafePointer<GenericEntity>> getEntities(const std::wstring &p_name);
		std::vector<spk::SafePointer<const GenericEntity>> getEntities(const std::wstring &p_name) const;

		spk::SafePointer<GenericEntity> getEntityByTag(const std::wstring &p_name);
		spk::SafePointer<const GenericEntity> getEntityByTag(const std::wstring &p_name) const;
		std::vector<spk::SafePointer<GenericEntity>> getEntitiesByTag(const std::wstring &p_name);
		std::vector<spk::SafePointer<const GenericEntity>> getEntitiesByTag(const std::wstring &p_name) const;

		bool contains(const std::wstring &p_name) const;

		size_t count(const std::wstring &p_name) const;

		void onGeometryChange(const spk::Geometry2D &p_geometry);
		void onPaintEvent(spk::PaintEvent &p_event);
		void onUpdateEvent(spk::UpdateEvent &p_event);
		void onKeyboardEvent(spk::KeyboardEvent &p_event);
		void onMouseEvent(spk::MouseEvent &p_event);
		void onControllerEvent(spk::ControllerEvent &p_event);
		void onTimerEvent(spk::TimerEvent &p_event);

	private:
		friend class GenericEntity;
	};
}
