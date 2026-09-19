#include "engine/entity.hpp"

#include <algorithm>
#include <vector>

#include "core/context/update_context.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	Entity::Entity(const std::string &name, Entity *parent) :
		NameTrait(name)
	{
		activate();
		_parentEditionContract = subscribeToParentEdition([this](const Entity *newParent) {
			_followParentContext(const_cast<Entity *>(newParent));
		});
		if (parent != nullptr)
		{
			setParent(*parent);
		}
		if (parent != nullptr)
		{
			setGeometry(parent->geometry());
		}
	}

	void Entity::_followParentContext(Entity *parent)
	{
		_parentContextEditionContract.resign();
		if (parent == nullptr)
		{
			changeContext(nullptr);
			return;
		}
		changeContext(parent->context());
		_parentContextEditionContract = parent->subscribeToContextEdition([this](Engine *const &, Engine *const &newContext) {
			changeContext(newContext);
		});
	}

	bool Entity::_isAcceptingEvent() const
	{
		return isEffectivelyActive();
	}

	bool Entity::_canUpdate() const
	{
		return isEffectivelyActive();
	}

	bool Entity::_canBuildRenderSnapshot() const
	{
		return isEffectivelyActive();
	}

	void Entity::_propagateEvent(
		const std::function<void(EventDispatcher *)> &callback)
	{
		const std::vector<Entity *> childSnapshot(children().begin(), children().end());
		for (Entity *child : childSnapshot)
		{
			if (child != nullptr && std::ranges::find(children(), child) != children().end())
			{
				callback(child);
			}
		}

		const auto behaviourSnapshot = BehaviourCollection::snapshotElements();
		for (const auto &snapshot : behaviourSnapshot)
		{
			if (BehaviourCollection::containsSnapshotElement(snapshot))
			{
				callback(snapshot.element);
			}
		}
	}

	void Entity::_onGeometryChange(const spk::Rect2D &)
	{
	}

	void Entity::_afterGeometryChange(const spk::Rect2D &geometry)
	{
		const auto componentSnapshot = ComponentCollection::snapshotElements();
		for (const auto &snapshot : componentSnapshot)
		{
			if (ComponentCollection::containsSnapshotElement(snapshot))
			{
				snapshot.element->setGeometry(geometry);
			}
		}

		const auto behaviourSnapshot = BehaviourCollection::snapshotElements();
		for (const auto &snapshot : behaviourSnapshot)
		{
			if (BehaviourCollection::containsSnapshotElement(snapshot))
			{
				snapshot.element->setGeometry(geometry);
			}
		}

		const std::vector<Entity *> childSnapshot(children().begin(), children().end());
		for (Entity *child : childSnapshot)
		{
			if (child != nullptr && std::ranges::find(children(), child) != children().end())
			{
				child->setGeometry(geometry);
			}
		}
	}

	bool Entity::isEffectivelyActive() const
	{
		return resolveInHierarchy([](const Entity &entity) {
			return entity.isActive();
		});
	}

	void Entity::_afterBuildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		const auto componentSnapshot = ComponentCollection::snapshotElements();
		for (const auto &snapshot : componentSnapshot)
		{
			if (ComponentCollection::containsSnapshotElement(snapshot))
			{
				snapshot.element->buildRenderSnapshot(builder);
			}
		}

		const auto behaviourSnapshot = BehaviourCollection::snapshotElements();
		for (const auto &snapshot : behaviourSnapshot)
		{
			if (BehaviourCollection::containsSnapshotElement(snapshot))
			{
				snapshot.element->buildRenderSnapshot(builder);
			}
		}

		const std::vector<Entity *> childSnapshot(children().begin(), children().end());
		for (Entity *child : childSnapshot)
		{
			if (child != nullptr && std::ranges::find(children(), child) != children().end())
			{
				child->buildRenderSnapshot(builder);
			}
		}
	}

	void Entity::_afterUpdate(UpdateContext &context)
	{
		const auto behaviourSnapshot = BehaviourCollection::snapshotElements();
		for (const auto &snapshot : behaviourSnapshot)
		{
			if (BehaviourCollection::containsSnapshotElement(snapshot))
			{
				snapshot.element->updateState(context);
			}
		}

		const std::vector<Entity *> childSnapshot(children().begin(), children().end());
		for (Entity *child : childSnapshot)
		{
			if (child != nullptr && std::ranges::find(children(), child) != children().end())
			{
				child->updateState(context);
			}
		}
	}
}
