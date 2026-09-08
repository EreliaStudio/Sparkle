#include "engine/engine.hpp"

#include <algorithm>
#include <vector>

#include "core/context/update_context.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	Engine::Engine() :
		_root("Engine root")
	{
		_root.changeContext(this);
	}

	Engine::~Engine()
	{
		while (!_root.children().empty())
		{
			_root.children().back()->setParent(nullptr);
		}
		// Include detached and nested entities registered with this engine,
		// not just direct root children. Context changes also notify attachments.
		while (!Registry<Engine *, Entity>::elements(this).empty())
		{
			(*Registry<Engine *, Entity>::elements(this).begin())->changeContext(nullptr);
		}
		while (!systems().empty())
		{
			removeSystem(*systems().back());
		}
	}

	bool Engine::_isAcceptingInteraction() const
	{
		return true;
	}

	void Engine::_propagateInteraction(
		const std::function<void(EventDispatcher *)> &callback)
	{
		callback(&_root);

		const auto systemSnapshot = SystemCollection::snapshotElements();
		for (const auto &snapshot : systemSnapshot)
		{
			if (SystemCollection::containsSnapshotElement(snapshot))
			{
				callback(snapshot.element);
			}
		}
	}

	void Engine::addEntity(Entity *entity)
	{
		if (entity == nullptr)
		{
			return;
		}

		entity->changeContext(this);
		entity->setParent(&_root);
		entity->handleGeometryChange(_geometry);
	}

	void Engine::removeEntity(Entity *entity)
	{
		if (entity == nullptr)
		{
			return;
		}

		entity->setParent(nullptr);
		entity->changeContext(nullptr);
	}

	void Engine::removeSystem(System &system)
	{
		system.attach(nullptr);
		unregisterSystem(system);
	}

	Entity &Engine::root() noexcept
	{
		return _root;
	}

	const Entity &Engine::root() const noexcept
	{
		return _root;
	}

	void Engine::handleGeometryChange(const spk::Rect2D &geometry)
	{
		_geometry = geometry;
		_root.handleGeometryChange(_geometry);
	}

	const spk::Rect2D &Engine::geometry() const noexcept
	{
		return _geometry;
	}

	void Engine::buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		_root.buildRenderSnapshot(builder);
	}

	void Engine::updateState(UpdateContext &context)
	{
		_root.updateState(context);

		const auto systemSnapshot = SystemCollection::snapshotElements();
		for (const auto &snapshot : systemSnapshot)
		{
			if (SystemCollection::containsSnapshotElement(snapshot))
			{
				snapshot.element->updateState(context);
			}
		}
	}
}
