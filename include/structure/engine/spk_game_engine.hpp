#pragma once

#include <vector>

#include "structure/engine/spk_entity.hpp"

namespace spk
{
	class GameEngine
	{
	private:
		std::vector<spk::SafePointer<Entity>> _entities;

	public:
		GameEngine() {}

		void addEntity(const spk::SafePointer<Entity>& p_entity)
		{
			_entities.push_back(p_entity);
		}

		void removeEntity(const spk::SafePointer<Entity>& p_entity)
		{
			auto it = std::find_if(_entities.begin(), _entities.end(),
				[&p_entity](const spk::SafePointer<Entity>& uniquePtr) {
					return uniquePtr.get() == p_entity.get();
				});

			if (it != _entities.end())
			{
				_entities.erase(it);
			}
		}

		spk::SafePointer<Entity> getEntity(const std::wstring& p_name)
		{
			for (const auto& entity : _entities)
			{
				if (entity->name() == p_name)
				{
					return entity.get();
				}
			}
			return nullptr;
		}

		std::vector<spk::SafePointer<Entity>> getEntities(const std::wstring& p_name)
		{
			std::vector<spk::SafePointer<Entity>> result;
			for (const auto& entity : _entities)
			{
				if (entity->name() == p_name)
				{
					result.push_back(entity.get());
				}
			}
			return result;
		}

		bool contains(const std::wstring& p_name) const
		{
			return std::any_of(_entities.begin(), _entities.end(),
				[&](const spk::SafePointer<Entity>& p_entity)
				{
					return p_entity->name() == p_name;
				});
		}

		size_t count(const std::wstring& p_name) const
		{
			return std::count_if(_entities.begin(), _entities.end(),
				[&](const spk::SafePointer<Entity>& p_entity)
				{
					return p_entity->name() == p_name;
				});
		}

		void update(const long long& p_deltaTime)
		{
			for (const auto& entity : _entities)
			{
				entity->update(p_deltaTime);
			}
		}

		void render()
		{
			for (const auto& entity : _entities)
			{
				entity->render();
			}
		}
	};
}