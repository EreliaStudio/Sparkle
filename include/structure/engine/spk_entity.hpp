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

		std::vector<std::unique_ptr<Component>> _components;
		Transform& _transform;

		spk::ActivableObject::Contract _awakeContract;
		spk::ActivableObject::Contract _sleepContract;

		spk::ActivableObject::Contract constructAwakeContract()
		{
			return addActivationCallback([&]() {
				for (auto& component : _components)
				{
					component->awake();
				}
				});
		}

		spk::ActivableObject::Contract constructSleepContract()
		{
			return addDeactivationCallback([&]() {
				for (auto& component : _components)
				{
					component->sleep();
				}
				});
		}

	public:
		Entity(const std::wstring& p_name, const spk::SafePointer<Entity>& p_owner = nullptr) :
			_name(p_name),
			_components(),
			_transform(addComponent<Transform>(this)),
			_awakeContract(constructAwakeContract()),
			_sleepContract(constructSleepContract())
		{
			if (p_owner != nullptr)
				p_owner->addChild(this);
		}

		Entity(const Entity&) = delete;
		Entity& operator=(const Entity&) = delete;

		~Entity()
		{
			for (auto& component : _components)
			{
				component->stop();
			}
		}

		void setName(const std::wstring& p_name)
		{
			_name = p_name;
		}

		const std::wstring& name() const
		{
			return (_name);
		}

		Transform& transform()
		{
			return (_transform);
		}

		const Transform& transform() const
		{
			return (_transform);
		}

		template <typename TComponentType, typename... TArgs>
		TComponentType& addComponent(TArgs&&... p_args)
		{
			static_assert(std::is_base_of_v<Component, TComponentType>, "TComponentType must inherit from Component");
			std::unique_ptr<TComponentType> newComponent = std::make_unique<TComponentType>(std::forward<TArgs>(p_args)...);
			TComponentType* result = newComponent.get();

			_components.emplace_back(std::move(newComponent));

			result->start();

			return (*result);
		}

		void removeComponent(const std::wstring& p_name)
		{
			auto it = std::remove_if(_components.begin(), _components.end(),
				[&](const std::unique_ptr<Component>& c) {
					return c->name() == p_name;
				});

			it->get()->stop();

			_components.erase(it, _components.end());
		}

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

		void render()
		{
			for (auto& component : _components)
			{
				component->render();
			}

			for (auto& child : children())
			{
				child->render();
			}
		}

		void update(const long long& p_duration)
		{
			for (auto& component : _components)
			{
				component->update(p_duration);
			}

			for (auto& child : children())
			{
				child->update(p_duration);
			}
		}
	};
}