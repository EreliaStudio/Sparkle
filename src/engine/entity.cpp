#include "engine/entity.hpp"

#include "core/context/update_context.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	Entity::Entity(const std::string &name, Entity *parent) :
		NameTrait(name)
	{
		activate();
		setParent(parent);
		if (parent != nullptr)
		{
			handleGeometryChange(parent->geometry());
		}
	}

	bool Entity::_isAcceptingInteraction() const
	{
		return isActive();
	}

	void Entity::_propagateInteraction(
		const std::function<void(EventDispatcher *)> &callback)
	{
		for (Entity *child : children())
		{
			if (child != nullptr)
			{
				callback(child);
			}
		}

		for (const auto &behaviour : behaviours())
		{
			if (behaviour != nullptr)
			{
				callback(behaviour.get());
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

		for (const auto &participant : participants())
		{
			if (participant != nullptr)
			{
				participant->handleGeometryChange(geometry);
			}
		}

		for (const auto &behaviour : behaviours())
		{
			if (behaviour != nullptr)
			{
				behaviour->handleGeometryChange(geometry);
			}
		}

		for (Entity *child : children())
		{
			if (child != nullptr)
			{
				child->handleGeometryChange(geometry);
			}
		}
	}

	const spk::Rect2D &Entity::geometry() const noexcept
	{
		return _geometry;
	}

	void Entity::buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		if (!isActive())
		{
			return;
		}

		_buildRenderSnapshot(builder);

		for (const auto &participant : participants())
		{
			if (participant != nullptr)
			{
				participant->buildRenderSnapshot(builder);
			}
		}

		for (const auto &behaviour : behaviours())
		{
			if (behaviour != nullptr)
			{
				behaviour->buildRenderSnapshot(builder);
			}
		}

		for (Entity *child : children())
		{
			if (child != nullptr)
			{
				child->buildRenderSnapshot(builder);
			}
		}
	}

	void Entity::updateState(UpdateContext &context)
	{
		if (!isActive())
		{
			return;
		}

		for (const auto &behaviour : behaviours())
		{
			if (behaviour != nullptr)
			{
				behaviour->updateState(context);
			}
		}

		for (Entity *child : children())
		{
			if (child != nullptr)
			{
				child->updateState(context);
			}
		}
	}
}
