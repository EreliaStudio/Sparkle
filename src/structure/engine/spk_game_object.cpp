#include "structure/engine/spk_game_object.hpp"

namespace spk
{
	spk::ActivableObject::Contract GameObject::constructAwakeContract()
	{
		return std::move(addActivationCallback(
			[&]()
			{
				for (auto &component : _components)
				{
					if (component->isActive() == true)
					{
						component->awake();
					}
				}
			}));
	}

	spk::ActivableObject::Contract GameObject::constructSleepContract()
	{
		return std::move(addDeactivationCallback(
			[&]()
			{
				for (auto &component : _components)
				{
					if (component->isActive() == true)
					{
						component->sleep();
					}
				}
			}));
	}

	GameObject::GameObject(const std::wstring &p_name, const spk::SafePointer<GameObject> &p_owner) :
		_name(p_name),
		_components(),
		_transform(addComponent<Transform>()),
		_awakeContract(constructAwakeContract()),
		_sleepContract(constructSleepContract())
	{
		_ownerTransformEditionContract = _transform->addOnEditionCallback(
			[&]()
			{
				for (auto &child : children())
				{
					child->transform()._updateModel();
				}
			});

		if (p_owner != nullptr)
		{
			p_owner->addChild(this);
		}
	}

	GameObject::GameObject() :
		GameObject(L"Unnamed entity")
	{
	}

	GameObject::~GameObject()
	{
		for (auto &component : _components)
		{
			component->stop();
		}
	}
	void GameObject::setName(const std::wstring &p_name)
	{
		_name = p_name;
	}

	void GameObject::setPriority(const int &p_priority)
	{
		_priority = p_priority;
		if (parent() != nullptr)
		{
			static_cast<GameObject *>(parent())->sortChildren();
		}
	}

	void GameObject::addTag(const std::wstring &p_tag)
	{
		_tags.insert(p_tag);
	}

	void GameObject::removeTag(const std::wstring &p_tag)
	{
		_tags.erase(p_tag);
	}

	void GameObject::clearTags()
	{
		_tags.clear();
	}

	bool GameObject::containTag(const std::wstring &p_tag) const
	{
		return _tags.find(p_tag) != _tags.end();
	}

	const std::wstring &GameObject::name() const
	{
		return (_name);
	}

	const std::set<std::wstring> &GameObject::tags() const
	{
		return (_tags);
	}

	const spk::Vector3 &GameObject::position() const
	{
		return (_transform->position());
	}

	int GameObject::priority() const
	{
		return (_priority);
	}

	Transform &GameObject::transform()
	{
		return (*_transform);
	}

	const Transform &GameObject::transform() const
	{
		return (*_transform);
	}

	void GameObject::removeAllComponents()
	{
		std::unique_lock<std::mutex> lock(_componentMutex);
		for (auto &component : _components)
		{
			component->stop();
		}
		_components.clear();
	}

	void GameObject::removeComponent(const std::wstring &p_name)
	{
		std::unique_lock<std::mutex> lock(_componentMutex);
		auto it = std::remove_if(_components.begin(), _components.end(), [&](const std::unique_ptr<Component> &p_c) { return p_c->name() == p_name; });

		it->get()->sleep();
		it->get()->stop();

		_components.erase(it, _components.end());
	}

	void GameObject::onPaintEvent(spk::PaintEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				component->onPaintEvent(p_event);
			}
		}

		sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onPaintEvent(p_event);
			}
		}
	}

	void GameObject::onUpdateEvent(spk::UpdateEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onUpdateEvent(p_event);
			}
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				component->onUpdateEvent(p_event);
			}
		}
	}

	void GameObject::onKeyboardEvent(spk::KeyboardEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onKeyboardEvent(p_event);
			}
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				component->onKeyboardEvent(p_event);
			}
		}
	}

	void GameObject::onMouseEvent(spk::MouseEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onMouseEvent(p_event);
			}
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				component->onMouseEvent(p_event);
			}
		}
	}

	void GameObject::onControllerEvent(spk::ControllerEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onControllerEvent(p_event);
			}
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				component->onControllerEvent(p_event);
			}
		}
	}

	void GameObject::onTimerEvent(spk::TimerEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onTimerEvent(p_event);
			}
		}

		sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				component->onTimerEvent(p_event);
			}
		}
	}

	void GameObject::sortChildren()
	{
		_needChildSorting = true;
	}

	void GameObject::sortComponent()
	{
		_needComponentSorting = true;
	}

	spk::SafePointer<GameObject> GameObject::getChild(const std::wstring &p_name)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		for (auto &child : children())
		{
			if (child->name() == p_name)
			{
				return (child);
			}
		}
		return (nullptr);
	}

	spk::SafePointer<const GameObject> GameObject::getChild(const std::wstring &p_name) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		for (const auto &child : children())
		{
			if (child->name() == p_name)
			{
				return (child);
			}
		}
		return (nullptr);
	}

	std::vector<spk::SafePointer<GameObject>> GameObject::getChildren(const std::wstring &p_name)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		std::vector<spk::SafePointer<GameObject>> result;

		for (auto &child : children())
		{
			if (child->name() == p_name)
			{
				result.push_back(child);
			}
		}
		return (result);
	}

	std::vector<spk::SafePointer<const GameObject>> GameObject::getChildren(const std::wstring &p_name) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		std::vector<spk::SafePointer<const GameObject>> result;

		for (const auto &child : children())
		{
			if (child->name() == p_name)
			{
				result.push_back(child);
			}
		}
		return (result);
	}

	bool GameObject::contains(const std::wstring &p_name) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return std::any_of(
			children().begin(), children().end(), [&](const spk::SafePointer<GameObject> &p_entity) { return p_entity->name() == p_name; });
	}

	size_t GameObject::count(const std::wstring &p_name) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return std::count_if(
			children().begin(), children().end(), [&](const spk::SafePointer<GameObject> &p_entity) { return p_entity->name() == p_name; });
	}

	spk::SafePointer<GameObject> GameObject::getChildByTag(const std::wstring &p_tag)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return getChildByTags(std::span<const std::wstring>(&p_tag, 1));
	}

	spk::SafePointer<const GameObject> GameObject::getChildByTag(const std::wstring &p_tag) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return getChildByTags(std::span<const std::wstring>(&p_tag, 1));
	}

	std::vector<spk::SafePointer<GameObject>> GameObject::getChildrenByTag(const std::wstring &p_tag)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return (getChildrenByTag({p_tag}));
	}

	std::vector<spk::SafePointer<const GameObject>> GameObject::getChildrenByTag(const std::wstring &p_tag) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return (getChildrenByTag({p_tag}));
	}

	bool GameObject::containsTag(const std::wstring &p_tag) const
	{
		return (contains({p_tag}));
	}

	size_t GameObject::countTag(const std::wstring &p_tag) const
	{
		return (count({p_tag}));
	}

	static bool matchesTags(const GameObject &p_entity, const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_op)
	{
		if (p_op == spk::BinaryOperator::AND)
		{
			return std::all_of(p_tags.begin(), p_tags.end(), [&](const std::wstring &p_tag) { return p_entity.containTag(p_tag); });
		}
		else
		{
			return std::any_of(p_tags.begin(), p_tags.end(), [&](const std::wstring &p_tag) { return p_entity.containTag(p_tag); });
		}
	}

	spk::SafePointer<GameObject> GameObject::getChildByTags(const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		for (auto &child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				return child;
			}
		}
		return nullptr;
	}

	spk::SafePointer<const GameObject> GameObject::getChildByTags(const std::span<const std::wstring> &p_tags,
																  spk::BinaryOperator p_binaryOperator) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		for (const auto &child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				return child;
			}
		}
		return nullptr;
	}

	std::vector<spk::SafePointer<GameObject>> GameObject::getChildrenByTags(const std::span<const std::wstring> &p_tags,
																			spk::BinaryOperator p_binaryOperator)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		std::vector<spk::SafePointer<GameObject>> result;

		for (auto &child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				result.push_back(child);
			}
		}
		return result;
	}

	std::vector<spk::SafePointer<const GameObject>> GameObject::getChildrenByTags(const std::span<const std::wstring> &p_tags,
																				  spk::BinaryOperator p_binaryOperator) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		std::vector<spk::SafePointer<const GameObject>> result;

		for (const auto &child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				result.push_back(child);
			}
		}
		return result;
	}

	bool GameObject::containsTags(const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return std::any_of(children().begin(),
						   children().end(),
						   [&](const spk::SafePointer<const GameObject> &p_child) { return matchesTags(*p_child, p_tags, p_binaryOperator); });
	}

	size_t GameObject::countTags(const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return std::count_if(children().begin(),
							 children().end(),
							 [&](const spk::SafePointer<const GameObject> &p_child) { return matchesTags(*p_child, p_tags, p_binaryOperator); });
	}
}