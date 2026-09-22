#include "rendering/engine/rendering_engine.hpp"

#include <algorithm>
#include <vector>

#include "rendering/engine/rendering_behaviour.hpp"
#include "rendering/engine/rendering_component.hpp"
#include "rendering/engine/rendering_entity.hpp"
#include "rendering/engine/rendering_system.hpp"

namespace spk
{
	RenderingEngine::RenderingEngine(Engine *engine) :
		_engine(engine)
	{
	}

	void RenderingEngine::setEngine(Engine *engine) noexcept
	{
		_engine = engine;
	}

	Engine *RenderingEngine::engine() noexcept
	{
		return _engine;
	}

	const Engine *RenderingEngine::engine() const noexcept
	{
		return _engine;
	}

	void RenderingEngine::_visitEntity(
		Entity &entity,
		const std::function<void(RenderingObjectTrait &)> &callback) const
	{
		if (auto *rendering = dynamic_cast<RenderingObjectTrait *>(&entity))
		{
			callback(*rendering);
		}

		const auto componentSnapshot = entity.getComponents<Component>();
		for (Component *component : componentSnapshot)
		{
			const auto currentComponents = entity.getComponents<Component>();
			if (std::ranges::find(currentComponents, component) == currentComponents.end())
			{
				continue;
			}

			if (auto *rendering = dynamic_cast<RenderingObjectTrait *>(component))
			{
				callback(*rendering);
			}
		}

		const auto behaviourSnapshot = entity.getBehaviours<Behaviour>();
		for (Behaviour *behaviour : behaviourSnapshot)
		{
			const auto currentBehaviours = entity.getBehaviours<Behaviour>();
			if (std::ranges::find(currentBehaviours, behaviour) == currentBehaviours.end())
			{
				continue;
			}

			if (auto *rendering = dynamic_cast<RenderingObjectTrait *>(behaviour))
			{
				callback(*rendering);
			}
		}

		const std::vector<Entity *> childSnapshot(
			entity.children().begin(),
			entity.children().end());
		for (Entity *child : childSnapshot)
		{
			if (child != nullptr &&
				std::ranges::find(entity.children(), child) != entity.children().end())
			{
				_visitEntity(*child, callback);
			}
		}
	}

	void RenderingEngine::_visitObjects(
		const std::function<void(RenderingObjectTrait &)> &callback) const
	{
		if (_engine == nullptr)
		{
			return;
		}

		_visitEntity(_engine->root(), callback);
		_engine->forEachSystem([&callback](System &system) {
			if (auto *rendering = dynamic_cast<RenderingObjectTrait *>(&system))
			{
				callback(*rendering);
			}
		});
	}

	bool RenderingEngine::_isAcceptingEvent() const
	{
		return _engine != nullptr;
	}

	void RenderingEngine::_propagateEvent(
		const std::function<void(EventDispatcher *)> &callback)
	{
		_visitObjects([&callback](RenderingObjectTrait &object) {
			callback(&object);
		});
	}

	bool RenderingEngine::_canUpdateByDevice() const
	{
		return _engine != nullptr;
	}

	void RenderingEngine::_afterUpdate(
		UpdateContext &context,
		DeviceContext &deviceContext)
	{
		_visitObjects([&](RenderingObjectTrait &object) {
			object.updateState(context, deviceContext);
		});
	}

	bool RenderingEngine::_canBuildRenderSnapshot() const
	{
		return _engine != nullptr;
	}

	void RenderingEngine::_afterBuildRenderSnapshot(
		RenderSnapshot::Builder &builder)
	{
		_visitObjects([&builder](RenderingObjectTrait &object) {
			object.buildRenderSnapshot(builder);
		});
	}
}
