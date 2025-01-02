#include "structure/engine/spk_entity.hpp"

namespace spk
{
	spk::ActivableObject::Contract Entity::constructAwakeContract()
	{
		return addActivationCallback([&]() {
			for (auto& component : _components)
			{
				if (component->isActive() == true)
				{
					component->awake();
				}
			}
			});
	}

	spk::ActivableObject::Contract Entity::constructSleepContract()
	{
		return addDeactivationCallback([&]() {
			for (auto& component : _components)
			{
				if (component->isActive() == true)
				{
					component->sleep();
				}
			}
			});
	}

	Entity::Entity(const std::wstring& p_name, const spk::SafePointer<Entity>& p_parent) :
		_name(p_name),
		_components(),
		_transform(addComponent<Transform>()),
		_awakeContract(constructAwakeContract()),
		_sleepContract(constructSleepContract())
	{
		_ownerTransformEditionContract = _transform.addOnEditionCallback([&](){
			for (auto& child : children())
			{
				child->transform()._updateModel();
			}			
		});
		activate();
		if (p_parent != nullptr)
		{
			p_parent->addChild(this);
		}
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
	
	void Entity::addTag(const std::wstring& p_tag)
	{
		_tags.insert(p_tag);
	}
	
	void Entity::removeTag(const std::wstring& p_tag)
	{
		_tags.erase(p_tag);
	}
	
	void Entity::clearTags()
	{
		_tags.clear();
	}
	
	bool Entity::containTag(const std::wstring& p_tag) const
	{
		return _tags.find(p_tag) != _tags.end();
	}

	const std::wstring& Entity::name() const
	{
		return (_name);
	}

	const std::set<std::wstring>& Entity::tags() const
	{
		return (_tags);
	}

	const spk::Vector3& Entity::position() const
	{
		return (_transform.position());
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

	void Entity::onPaintEvent(spk::PaintEvent& p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return ;
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		for (auto& component : _components)
		{
			component->onPaintEvent(p_event);
		}

		sortByPriority(children(), _needChildSorting, _childMutex);

		for (auto& child : children())
		{
			child->onPaintEvent(p_event);
		}
	}
	
	void Entity::onUpdateEvent(spk::UpdateEvent& p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return ;
		}

		sortByPriority(children(), _needChildSorting, _childMutex);

		for (auto& child : children())
		{
			child->onUpdateEvent(p_event);
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		for (auto& component : _components)
		{
			component->onUpdateEvent(p_event);
		}
	}
	
	void Entity::onKeyboardEvent(spk::KeyboardEvent& p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return ;
		}
		
		sortByPriority(children(), _needChildSorting, _childMutex);

		for (auto& child : children())
		{
			child->onKeyboardEvent(p_event);
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		for (auto& component : _components)
		{
			component->onKeyboardEvent(p_event);
		}
	}
	
	void Entity::onMouseEvent(spk::MouseEvent& p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return ;
		}
		
		sortByPriority(children(), _needChildSorting, _childMutex);

		for (auto& child : children())
		{
			child->onMouseEvent(p_event);
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		for (auto& component : _components)
		{
			component->onMouseEvent(p_event);
		}
	}

	void Entity::onControllerEvent(spk::ControllerEvent& p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return ;
		}
		
		sortByPriority(children(), _needChildSorting, _childMutex);

		for (auto& child : children())
		{
			child->onControllerEvent(p_event);
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		for (auto& component : _components)
		{
			component->onControllerEvent(p_event);
		}
	}
	
	void Entity::onTimerEvent(spk::TimerEvent& p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return ;
		}
		
		sortByPriority(children(), _needChildSorting, _childMutex);

		for (auto& child : children())
		{
			child->onTimerEvent(p_event);
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		for (auto& component : _components)
		{
			component->onTimerEvent(p_event);
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

	spk::SafePointer<Entity> Entity::getChildByTag(const std::wstring& p_tag)
	{
		return getChildByTags(std::span<const std::wstring>(&p_tag, 1));
	}

	spk::SafePointer<const Entity> Entity::getChildByTag(const std::wstring& p_tag) const
	{
		return getChildByTags(std::span<const std::wstring>(&p_tag, 1));
	}
	
	std::vector<spk::SafePointer<Entity>> Entity::getChildrenByTag(const std::wstring& p_tag)
	{
		return (getChildrenByTag({p_tag}));
	}
	
	std::vector<spk::SafePointer<const Entity>> Entity::getChildrenByTag(const std::wstring& p_tag) const
	{
		return (getChildrenByTag({p_tag}));
	}
	
	bool Entity::containsTag(const std::wstring& p_tag) const
	{
		return (contains({p_tag}));
	}
	
	size_t Entity::countTag(const std::wstring& p_tag) const
	{
		return (count({p_tag}));
	}

	static bool matchesTags(const Entity& entity, const std::span<const std::wstring>& tags, spk::BinaryOperator op)
	{
		if (op == spk::BinaryOperator::AND)
		{
			return std::all_of(tags.begin(), tags.end(), [&](const std::wstring& tag) {
				return entity.containTag(tag);
			});
		}
		else // spk::BinaryOperator::OR
		{
			return std::any_of(tags.begin(), tags.end(), [&](const std::wstring& tag) {
				return entity.containTag(tag);
			});
		}
	}

	spk::SafePointer<Entity> Entity::getChildByTags(const std::span<const std::wstring>& p_tags, spk::BinaryOperator p_binaryOperator)
	{
		for (auto& child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				return child;
			}
		}
		return nullptr;
	}

	spk::SafePointer<const Entity> Entity::getChildByTags(const std::span<const std::wstring>& p_tags, spk::BinaryOperator p_binaryOperator) const
	{
		for (const auto& child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				return child;
			}
		}
		return nullptr;
	}

	std::vector<spk::SafePointer<Entity>> Entity::getChildrenByTags(const std::span<const std::wstring>& p_tags, spk::BinaryOperator p_binaryOperator)
	{
		std::vector<spk::SafePointer<Entity>> result;

		for (auto& child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				result.push_back(child);
			}
		}
		return result;
	}

	std::vector<spk::SafePointer<const Entity>> Entity::getChildrenByTags(const std::span<const std::wstring>& p_tags, spk::BinaryOperator p_binaryOperator) const
	{
		std::vector<spk::SafePointer<const Entity>> result;

		for (const auto& child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				result.push_back(child);
			}
		}
		return result;
	}

	bool Entity::containsTags(const std::span<const std::wstring>& p_tags, spk::BinaryOperator p_binaryOperator) const
	{
		return std::any_of(children().begin(), children().end(),
			[&](const spk::SafePointer<const Entity>& child)
			{
				return matchesTags(*child, p_tags, p_binaryOperator);
			});
	}

	size_t Entity::countTags(const std::span<const std::wstring>& p_tags, spk::BinaryOperator p_binaryOperator) const
	{
		return std::count_if(children().begin(), children().end(),
			[&](const spk::SafePointer<const Entity>& child)
			{
				return matchesTags(*child, p_tags, p_binaryOperator);
			});
	}
}