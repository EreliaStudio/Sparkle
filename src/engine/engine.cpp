#include "engine/engine.hpp"

#include "core/context/update_context.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	Engine::Engine() :
		_root("Engine root")
	{
		_root.changeContext(this);
	}

	bool Engine::_isAcceptingInteraction() const
	{
		return true;
	}

	void Engine::_propagateInteraction(
		const std::function<void(EventDispatcher *)> &callback)
	{
		callback(&_root);

		for (const auto &system : systems())
		{
			if (system != nullptr)
			{
				callback(system.get());
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

		for (const auto &system : systems())
		{
			if (system != nullptr)
			{
				system->updateState(context);
			}
		}
	}
}
