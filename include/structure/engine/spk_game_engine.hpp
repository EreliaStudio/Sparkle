#pragma once

#include <vector>

#include "structure/engine/spk_entity.hpp"

namespace spk
{
	class GameEngine
	{
	private:
		Entity _centralEntity;

	public:
		GameEngine();

		void addEntity(const spk::SafePointer<Entity>& p_entity);
		void removeEntity(const spk::SafePointer<Entity>& p_entity);

		spk::SafePointer<Entity> getEntity(const std::wstring& p_name);
		spk::SafePointer<const Entity> getEntity(const std::wstring& p_name) const;
		std::vector<spk::SafePointer<Entity>> getEntities(const std::wstring& p_name);
		std::vector<spk::SafePointer<const Entity>> getEntities(const std::wstring& p_name) const;

		spk::SafePointer<Entity> getEntityByTag(const std::wstring& p_name);
		spk::SafePointer<const Entity> getEntityByTag(const std::wstring& p_name) const;
		std::vector<spk::SafePointer<Entity>> getEntitiesByTag(const std::wstring& p_name);
		std::vector<spk::SafePointer<const Entity>> getEntitiesByTag(const std::wstring& p_name) const;

		bool contains(const std::wstring& p_name) const;

		size_t count(const std::wstring& p_name) const;

		void update(const long long& p_deltaTime);

		void render();
	};
}