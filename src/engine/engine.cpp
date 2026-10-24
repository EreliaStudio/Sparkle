#include "engine/engine.hpp"

#include "core/context/update_context.hpp"

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
			_root.children().back()->clearParent();
		}

		Registry<Entity, Engine *> &entityRegistry = Registry<Entity, Engine *>::instance();
		while (!entityRegistry.elements(this).empty())
		{
			(*entityRegistry.elements(this).begin())->changeContext(nullptr);
		}

		while (!systems().empty())
		{
			removeSystem(*systems().back());
		}
	}

	void Engine::addEntity(Entity *entity)
	{
		if (entity == nullptr)
		{
			return;
		}

		entity->changeContext(this);
		entity->setParent(_root);
		entity->setGeometry(_geometry);
	}

	void Engine::removeEntity(Entity *entity)
	{
		if (entity == nullptr)
		{
			return;
		}

		entity->clearParent();
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
		_root.setGeometry(_geometry);
	}

	const spk::Rect2D &Engine::geometry() const noexcept
	{
		return _geometry;
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
