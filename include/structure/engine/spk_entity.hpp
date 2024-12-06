#pragma once

#include "structure/design_pattern/spk_inherence_object.hpp"
#include "structure/design_pattern/spk_activable_object.hpp"

#include <string>
#include "structure/engine/spk_transform.hpp"

namespace spk
{
	class Entity : public spk::InherenceObject<Entity>, public spk::ActivableObject
	{
	private:
		std::wstring _name;

		int _priority;
		
		mutable std::mutex _componentMutex;
		mutable std::mutex _childMutex;

		bool _needChildSorting = false;
		bool _needComponentSorting = false;
		std::vector<std::unique_ptr<Component>> _components;
		Transform& _transform;

		spk::ActivableObject::Contract _awakeContract;
		spk::ActivableObject::Contract _sleepContract;

		spk::ActivableObject::Contract constructAwakeContract();
		spk::ActivableObject::Contract constructSleepContract();

		template <typename T>
		void sortByPriority(std::vector<T>& p_container, bool& p_needSorting, std::mutex& p_mutex)
		{
        	std::unique_lock<std::mutex> mutexLock(p_mutex);

			if (p_needSorting == false)
			{
				return;
			}

			std::sort(p_container.begin(), p_container.end(), [](const T& a, const T& b) {
				int priorityA = a->priority();
				int priorityB = b->priority();

				bool aNoPriority = (priorityA < 0);
				bool bNoPriority = (priorityB < 0);

				if (aNoPriority && bNoPriority)
				{	
					return false;
				}

				if (aNoPriority != bNoPriority)
				{
					return !aNoPriority;
				}

				return priorityA > priorityB;
			});

			p_needSorting = false;
		}

	public:
		Entity(const std::wstring& p_name, const spk::SafePointer<Entity>& p_owner = nullptr);

		Entity();

		Entity(const Entity&) = delete;
		Entity& operator=(const Entity&) = delete;

		~Entity();

		void setName(const std::wstring& p_name);
		void setPriority(const int& p_priority);

		const std::wstring& name() const;
		int priority() const;
		Transform& transform();
		const Transform& transform() const;

		template <typename TComponentType, typename... TArgs>
		TComponentType& addComponent(TArgs&&... p_args)
		{
			static_assert(std::is_base_of_v<Component, TComponentType>, "TComponentType must inherit from Component");
			std::unique_ptr<TComponentType> newComponent = std::make_unique<TComponentType>(std::forward<TArgs>(p_args)...);
			TComponentType* result = newComponent.get();

			{
				std::unique_lock<std::mutex> lock(_componentMutex);
			
				_components.emplace_back(std::move(newComponent));
			}

			newComponent->_owner = this;
			result->start();

			if (isActive() == true)
			{
				result->awake();
			}

			return (*result);
		}

		void removeComponent(const std::wstring& p_name);

		template <typename TComponentType>
		TComponentType& getComponent(const std::wstring& p_name = L"")
		{
			for (auto* component : _components)
			{
				TComponentType* castedComponent = dynamic_cast<TComponentType*>(component);
				if (castedComponent != nullptr)
				{
					if (p_name.empty() || castedComponent->name() == p_name)
					{
						return *castedComponent;
					}
				}
			}

			throw std::runtime_error("Component of specified type and name not found.");
		}

		template <typename TComponentType>
		std::vector<spk::SafePointer<TComponentType>> getComponents()
		{
			std::vector<spk::SafePointer<TComponentType>> result;

			for (auto* component : _components)
			{
				TComponentType* castedComponent = dynamic_cast<TComponentType*>(component);
				if (castedComponent != nullptr)
				{
					result.emplace_back(castedComponent);
				}
			}

			return result;
		}

		template <typename TComponentType>
		const TComponentType& getComponent(const std::wstring& p_name = L"") const
		{
			for (auto* component : _components)
			{
				TComponentType* castedComponent = dynamic_cast<TComponentType*>(component);
				if (castedComponent != nullptr)
				{
					if (p_name.empty() || castedComponent->name() == p_name)
					{
						return *castedComponent;
					}
				}
			}

			throw std::runtime_error("Component of specified type and name not found.");
		}

		template <typename TComponentType>
		std::vector<spk::SafePointer<const TComponentType>> getComponents() const
		{
			std::vector<spk::SafePointer<const TComponentType>> result;

			for (auto* component : _components)
			{
				TComponentType* castedComponent = dynamic_cast<TComponentType*>(component);
				if (castedComponent != nullptr)
				{
					result.emplace_back(castedComponent);
				}
			}

			return result;
		}

		void render();
		void update(const long long& p_duration);
		void sortChildren();
		void sortComponent();

		spk::SafePointer<Entity> getChild(const std::wstring& p_name);
		spk::SafePointer<const Entity> getChild(const std::wstring& p_name) const;
		std::vector<spk::SafePointer<Entity>> getChildren(const std::wstring& p_name);
		std::vector<spk::SafePointer<const Entity>> getChildren(const std::wstring& p_name) const;
		bool contains(const std::wstring& p_name) const;
		size_t count(const std::wstring& p_name) const;
	};
}