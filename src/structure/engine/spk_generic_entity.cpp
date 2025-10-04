#include "structure/engine/3d/spk_collider.hpp"
#include "structure/engine/3d/spk_entity.hpp"
#include "structure/engine/spk_game_engine.hpp"

namespace spk
{
	GenericEntity::GenericEntity(const std::wstring &p_name, const spk::SafePointer<GenericEntity> &p_owner) :
		_name(p_name),
		_components()
	{
		if (p_owner != nullptr)
		{
			p_owner->addChild(this);
		}
	}

	GenericEntity::GenericEntity() :
		GenericEntity(L"Unnamed entity")
	{
	}

	GenericEntity::~GenericEntity()
	{
		for (auto &component : _components)
		{
			component->stop();
		}
	}
	void GenericEntity::setName(const std::wstring &p_name)
	{
		_name = p_name;
	}

	void GenericEntity::setPriority(const int &p_priority)
	{
		_priority = p_priority;
		if (parent() != nullptr)
		{
			static_cast<GenericEntity *>(parent())->sortChildren();
		}
	}

	void GenericEntity::addTag(const std::wstring &p_tag)
	{
		_tags.insert(p_tag);
	}

	void GenericEntity::removeTag(const std::wstring &p_tag)
	{
		_tags.erase(p_tag);
	}

	void GenericEntity::clearTags()
	{
		_tags.clear();
	}

	bool GenericEntity::containTag(const std::wstring &p_tag) const
	{
		return (_tags.find(p_tag) != _tags.end());
	}

	void GenericEntity::setEngine(GameEngine *p_engine)
	{
		_engine = p_engine;
		for (auto &child : children())
		{
			child->setEngine(p_engine);
		}
	}

	spk::SafePointer<GameEngine> GenericEntity::engine()
	{
		return (_engine);
	}

	spk::SafePointer<const GameEngine> GenericEntity::engine() const
	{
		return (_engine);
	}

	void GenericEntity::requestPaint()
	{
		_requestPaint = true;
	}

	const std::wstring &GenericEntity::name() const
	{
		return (_name);
	}

	const std::set<std::wstring> &GenericEntity::tags() const
	{
		return (_tags);
	}

	int GenericEntity::priority() const
	{
		return (_priority);
	}

	void GenericEntity::removeAllComponents()
	{
		std::unique_lock<std::mutex> lock(_componentMutex);
		for (auto &component : _components)
		{
			component->stop();
		}
		_components.clear();
	}

	void GenericEntity::addChild(spk::SafePointer<GenericEntity> p_child)
	{
		spk::InherenceObject<GenericEntity>::addChild(p_child);
		p_child->setEngine(_engine);
	}

	void GenericEntity::removeChild(spk::SafePointer<GenericEntity> p_child)
	{
		spk::InherenceObject<GenericEntity>::removeChild(p_child);
		p_child->setEngine(nullptr);
	}

	void GenericEntity::removeChild(InherenceObject<GenericEntity>::Child p_child)
	{
		spk::InherenceObject<GenericEntity>::removeChild(p_child);
		static_cast<GenericEntity *>(p_child)->setEngine(nullptr);
	}

	void GenericEntity::clearChildren()
	{
		for (auto &child : children())
		{
			child->setEngine(nullptr);
		}
		spk::InherenceObject<GenericEntity>::clearChildren();
	}

	void GenericEntity::removeComponent(const std::wstring &p_name)
	{
		std::unique_lock<std::mutex> lock(_componentMutex);
		auto it =
			std::remove_if(_components.begin(), _components.end(), [&](const std::unique_ptr<Component> &p_c) { return (p_c->name() == p_name); });

		it->get()->sleep();
		it->get()->stop();

		_components.erase(it, _components.end());
	}

	void GenericEntity::onGeometryChange(const spk::Geometry2D &p_geometry)
	{
		_sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				component->onGeometryChange(p_geometry);
			}
		}

		_sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onGeometryChange(p_geometry);
			}
		}
	}

	void GenericEntity::onPaintEvent(spk::PaintEvent &p_event)
	{
		if (isActive() == false || p_event.consume == true)
		{
			return;
		}

		_sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				if (component->isActive() == true)
				{
					component->onPaintEvent(p_event);
				}
			}
		}

		_sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onPaintEvent(p_event);
			}
		}
	}

	void GenericEntity::onUpdateEvent(spk::UpdateEvent &p_event)
	{
		if (isActive() == false || p_event.consume == true)
		{
			return;
		}

		_sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onUpdateEvent(p_event);
			}
		}

		_sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				if (component->isActive() == true)
				{
					component->onUpdateEvent(p_event);
				}
			}
		}

		if (_requestPaint == true)
		{
			p_event.requestPaint();
			_requestPaint = false;
		}
	}

	void GenericEntity::onKeyboardEvent(spk::KeyboardEvent &p_event)
	{
		if (isActive() == false || p_event.consume == true)
		{
			return;
		}

		_sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onKeyboardEvent(p_event);
			}
		}

		_sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				if (component->isActive() == true)
				{
					component->onKeyboardEvent(p_event);
				}
			}
		}
	}

	void GenericEntity::onMouseEvent(spk::MouseEvent &p_event)
	{
		if (isActive() == false || p_event.consume == true)
		{
			return;
		}

		_sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onMouseEvent(p_event);
			}
		}

		_sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				if (component->isActive() == true)
				{
					component->onMouseEvent(p_event);
				}
			}
		}
	}

	void GenericEntity::onControllerEvent(spk::ControllerEvent &p_event)
	{
		if (isActive() == false || p_event.consume == true)
		{
			return;
		}

		_sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onControllerEvent(p_event);
			}
		}

		_sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				if (component->isActive() == true)
				{
					component->onControllerEvent(p_event);
				}
			}
		}
	}

	void GenericEntity::onTimerEvent(spk::TimerEvent &p_event)
	{
		if (isActive() == false || p_event.consume == true)
		{
			return;
		}

		_sortByPriority(children(), _needChildSorting, _childMutex);

		{
			std::unique_lock<std::mutex> lock(_childMutex);
			for (auto &child : children())
			{
				child->onTimerEvent(p_event);
			}
		}

		_sortByPriority(_components, _needComponentSorting, _componentMutex);

		{
			std::unique_lock<std::mutex> lock(_componentMutex);
			for (auto &component : _components)
			{
				if (component->isActive() == true)
				{
					component->onTimerEvent(p_event);
				}
			}
		}
	}

	void GenericEntity::sortChildren()
	{
		_needChildSorting = true;
	}

	void GenericEntity::sortComponent()
	{
		_needComponentSorting = true;
	}

	spk::SafePointer<GenericEntity> GenericEntity::getChild(const std::wstring &p_name)
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

	spk::SafePointer<const GenericEntity> GenericEntity::getChild(const std::wstring &p_name) const
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

	std::vector<spk::SafePointer<GenericEntity>> GenericEntity::getChildren(const std::wstring &p_name)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		std::vector<spk::SafePointer<GenericEntity>> result;

		for (auto &child : children())
		{
			if (child->name() == p_name)
			{
				result.push_back(child);
			}
		}
		return (result);
	}

	std::vector<spk::SafePointer<const GenericEntity>> GenericEntity::getChildren(const std::wstring &p_name) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		std::vector<spk::SafePointer<const GenericEntity>> result;

		for (const auto &child : children())
		{
			if (child->name() == p_name)
			{
				result.push_back(child);
			}
		}
		return (result);
	}

	bool GenericEntity::contains(const std::wstring &p_name) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return (std::any_of(
			children().begin(), children().end(), [&](const spk::SafePointer<GenericEntity> &p_entity) { return (p_entity->name() == p_name); }));
	}

	size_t GenericEntity::count(const std::wstring &p_name) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return (std::count_if(
			children().begin(), children().end(), [&](const spk::SafePointer<GenericEntity> &p_entity) { return (p_entity->name() == p_name); }));
	}

	spk::SafePointer<GenericEntity> GenericEntity::getChildByTag(const std::wstring &p_tag)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return (getChildByTags(std::span<const std::wstring>(&p_tag, 1)));
	}

	spk::SafePointer<const GenericEntity> GenericEntity::getChildByTag(const std::wstring &p_tag) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return (getChildByTags(std::span<const std::wstring>(&p_tag, 1)));
	}

	std::vector<spk::SafePointer<GenericEntity>> GenericEntity::getChildrenByTag(const std::wstring &p_tag)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return (getChildrenByTag({p_tag}));
	}

	std::vector<spk::SafePointer<const GenericEntity>> GenericEntity::getChildrenByTag(const std::wstring &p_tag) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return (getChildrenByTag({p_tag}));
	}

	bool GenericEntity::containsTag(const std::wstring &p_tag) const
	{
		return (contains({p_tag}));
	}

	size_t GenericEntity::countTag(const std::wstring &p_tag) const
	{
		return (count({p_tag}));
	}

	static bool matchesTags(const GenericEntity &p_entity, const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_op)
	{
		if (p_op == spk::BinaryOperator::AND)
		{
			return (std::all_of(p_tags.begin(), p_tags.end(), [&](const std::wstring &p_tag) { return (p_entity.containTag(p_tag)); }));
		}
		else
		{
			return (std::any_of(p_tags.begin(), p_tags.end(), [&](const std::wstring &p_tag) { return (p_entity.containTag(p_tag)); }));
		}
	}

	spk::SafePointer<GenericEntity> GenericEntity::getChildByTags(const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		for (auto &child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				return (child);
			}
		}
		return (nullptr);
	}

	spk::SafePointer<const GenericEntity> GenericEntity::getChildByTags(
		const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		for (const auto &child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				return (child);
			}
		}
		return (nullptr);
	}

	std::vector<spk::SafePointer<GenericEntity>> GenericEntity::getChildrenByTags(
		const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator)
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		std::vector<spk::SafePointer<GenericEntity>> result;

		for (auto &child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				result.push_back(child);
			}
		}
		return (result);
	}

	std::vector<spk::SafePointer<const GenericEntity>> GenericEntity::getChildrenByTags(
		const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		std::vector<spk::SafePointer<const GenericEntity>> result;

		for (const auto &child : children())
		{
			if (matchesTags(*child, p_tags, p_binaryOperator))
			{
				result.push_back(child);
			}
		}
		return (result);
	}

	bool GenericEntity::containsTags(const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return (std::any_of(
			children().begin(),
			children().end(),
			[&](const spk::SafePointer<const GenericEntity> &p_child) { return (matchesTags(*p_child, p_tags, p_binaryOperator)); }));
	}

	size_t GenericEntity::countTags(const std::span<const std::wstring> &p_tags, spk::BinaryOperator p_binaryOperator) const
	{
		std::unique_lock<std::mutex> lock(_childMutex);
		return (std::count_if(
			children().begin(),
			children().end(),
			[&](const spk::SafePointer<const GenericEntity> &p_child) { return (matchesTags(*p_child, p_tags, p_binaryOperator)); }));
	}
}