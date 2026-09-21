#include "rendering/engine/rendering_engine.hpp"

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

		for (Component *component : entity.getComponents<Component>())
		{
			if (auto *rendering = dynamic_cast<RenderingObjectTrait *>(component))
			{
				callback(*rendering);
			}
		}

		for (Behaviour *behaviour : entity.getBehaviours<Behaviour>())
		{
			if (auto *rendering = dynamic_cast<RenderingObjectTrait *>(behaviour))
			{
				callback(*rendering);
			}
		}

		for (Entity *child : entity.children())
		{
			if (child != nullptr)
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
		for (System *system : _engine->getSystems<System>())
		{
			if (auto *rendering = dynamic_cast<RenderingObjectTrait *>(system))
			{
				callback(*rendering);
			}
		}
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
