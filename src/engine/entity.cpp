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
		setParent(parent);
		if (parent != nullptr)
		{
			handleGeometryChange(parent->geometry());
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

	bool Entity::_isAcceptingInteraction() const
	{
		return isEffectivelyActive();
	}

	void Entity::_propagateInteraction(
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

	void Entity::_buildRenderSnapshot(spk::RenderSnapshot::Builder &)
	{
	}

	void Entity::handleGeometryChange(const spk::Rect2D &geometry)
	{
		_geometry = geometry;
		_onGeometryChange(_geometry);

		const auto participantSnapshot = SystemParticipantCollection::snapshotElements();
		for (const auto &snapshot : participantSnapshot)
		{
			if (SystemParticipantCollection::containsSnapshotElement(snapshot))
			{
				snapshot.element->handleGeometryChange(geometry);
			}
		}

		const auto behaviourSnapshot = BehaviourCollection::snapshotElements();
		for (const auto &snapshot : behaviourSnapshot)
		{
			if (BehaviourCollection::containsSnapshotElement(snapshot))
			{
				snapshot.element->handleGeometryChange(geometry);
			}
		}

		const std::vector<Entity *> childSnapshot(children().begin(), children().end());
		for (Entity *child : childSnapshot)
		{
			if (child != nullptr && std::ranges::find(children(), child) != children().end())
			{
				child->handleGeometryChange(geometry);
			}
		}
	}

	const spk::Rect2D &Entity::geometry() const noexcept
	{
		return _geometry;
	}

	bool Entity::isEffectivelyActive() const
	{
		return resolveInHierarchy([](const Entity &entity) {
			return entity.isActive();
		});
	}

	void Entity::buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		if (!isEffectivelyActive())
		{
			return;
		}

		_buildRenderSnapshot(builder);

		const auto participantSnapshot = SystemParticipantCollection::snapshotElements();
		for (const auto &snapshot : participantSnapshot)
		{
			if (SystemParticipantCollection::containsSnapshotElement(snapshot))
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

	void Entity::updateState(UpdateContext &context)
	{
		if (!isEffectivelyActive())
		{
			return;
		}

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
