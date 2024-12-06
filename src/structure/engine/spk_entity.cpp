#include "structure/engine/spk_entity.hpp"

namespace spk
{
	spk::ActivableObject::Contract Entity::constructAwakeContract()
	{
		return addActivationCallback([&]() {
			for (auto& component : _components)
			{
				component->awake();
			}
			});
	}

	spk::ActivableObject::Contract Entity::constructSleepContract()
	{
		return addDeactivationCallback([&]() {
			for (auto& component : _components)
			{
				component->sleep();
			}
			});
	}

	Entity::Entity(const std::wstring& p_name, const spk::SafePointer<Entity>& p_owner) :
		_name(p_name),
		_components(),
		_transform(addComponent<Transform>()),
		_awakeContract(constructAwakeContract()),
		_sleepContract(constructSleepContract())
	{
		if (p_owner != nullptr)
			p_owner->addChild(this);
	}

	Entity::Entity() :
		Entity(L"Unnamed entity")
	{

	}

	Entity::~Entity()
	{
		for (auto& component : _components)
		{
			component->stop();
		}
	}

	void Entity::setName(const std::wstring& p_name)
	{
		_name = p_name;
	}

	void Entity::setPriority(const int& p_priority)
	{
		_priority = p_priority;
		if (parent() != nullptr)
		{
			static_cast<Entity*>(parent())->sortChildren();
		}
	}

	const std::wstring& Entity::name() const
	{
		return (_name);
	}

	int Entity::priority() const
	{
		return (_priority);
	}

	Transform& Entity::transform()
	{
		return (_transform);
	}

	const Transform& Entity::transform() const
	{
		return (_transform);
	}

	void Entity::removeComponent(const std::wstring& p_name)
	{
		auto it = std::remove_if(_components.begin(), _components.end(),
			[&](const std::unique_ptr<Component>& c) {
				return c->name() == p_name;
			});

		it->get()->sleep();
		it->get()->stop();

		_components.erase(it, _components.end());
	}

	void Entity::render()
	{
		sortByPriority(_components, _needComponentSorting, _componentMutex);

		for (auto& component : _components)
		{
			component->render();
		}

		sortByPriority(children(), _needChildSorting, _childMutex);

		for (auto& child : children())
		{
			child->render();
		}
	}

	void Entity::update(const long long& p_duration)
	{
		sortByPriority(_components, _needComponentSorting, _componentMutex);

		for (auto& component : _components)
		{
			component->update(p_duration);
		}

		sortByPriority(children(), _needChildSorting, _childMutex);

		for (auto& child : children())
		{
			child->update(p_duration);
		}
	}

	void Entity::sortChildren()
	{
		_needChildSorting = true;
	}

	void Entity::sortComponent()
	{
		_needComponentSorting = true;
	}

	spk::SafePointer<Entity> Entity::getChild(const std::wstring& p_name)
	{
		for (auto& child : children())
		{
			if (child->name() == p_name)
			{
				return (child);
			}
		}
		return (nullptr);
	}

	spk::SafePointer<const Entity> Entity::getChild(const std::wstring& p_name) const
	{
		for (const auto& child : children())
		{
			if (child->name() == p_name)
			{
				return (child);
			}
		}
		return (nullptr);
	}

	std::vector<spk::SafePointer<Entity>> Entity::getChildren(const std::wstring& p_name)
	{
		std::vector<spk::SafePointer<Entity>> result;

		for (auto& child : children())
		{
			if (child->name() == p_name)
			{
				result.push_back(child);
			}
		}
		return (result);
	}

	std::vector<spk::SafePointer<const Entity>> Entity::getChildren(const std::wstring& p_name) const
	{
		std::vector<spk::SafePointer<const Entity>> result;

		for (const auto& child : children())
		{
			if (child->name() == p_name)
			{
				result.push_back(child);
			}
		}
		return (result);
	}

	bool Entity::contains(const std::wstring& p_name) const
	{
		return std::any_of(children().begin(), children().end(),
			[&](const spk::SafePointer<Entity>& p_entity)
			{
				return p_entity->name() == p_name;
			});
	}

	size_t Entity::count(const std::wstring& p_name) const
	{
		return std::count_if(children().begin(), children().end(),
			[&](const spk::SafePointer<Entity>& p_entity)
			{
				return p_entity->name() == p_name;
			});
	}
}