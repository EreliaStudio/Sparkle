#include "rendering/engine/engine.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <limits>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "rendering/command/execute_snapshot_render_command.hpp"
#include "rendering/engine/rendering_behaviour.hpp"
#include "rendering/engine/rendering_component.hpp"
#include "rendering/engine/rendering_entity.hpp"
#include "rendering/engine/rendering_system.hpp"

namespace spk
{
	void Engine::_visitEntity(Entity &entity, const std::function<void(EngineView::Contributor &)> &callback) const
	{
		if (auto *rendering = dynamic_cast<EngineView::Contributor *>(&entity))
		{
			callback(*rendering);
		}

		const auto componentSnapshot = entity.getComponents<Component>();
		for (Component *component : componentSnapshot)
		{
			const auto current = entity.getComponents<Component>();
			if (std::ranges::find(current, component) == current.end())
			{
				continue;
			}
			if (auto *rendering = dynamic_cast<EngineView::Contributor *>(component))
			{
				callback(*rendering);
			}
		}

		const auto behaviourSnapshot = entity.getBehaviours<Behaviour>();
		for (Behaviour *behaviour : behaviourSnapshot)
		{
			const auto current = entity.getBehaviours<Behaviour>();
			if (std::ranges::find(current, behaviour) == current.end())
			{
				continue;
			}
			if (auto *rendering = dynamic_cast<EngineView::Contributor *>(behaviour))
			{
				callback(*rendering);
			}
		}

		const std::vector<Entity *> children(entity.children().begin(), entity.children().end());
		for (Entity *child : children)
		{
			if (child != nullptr && std::ranges::find(entity.children(), child) != entity.children().end())
			{
				_visitEntity(*child, callback);
			}
		}
	}

	void Engine::_visitObjects(const std::function<void(EngineView::Contributor &)> &callback)
	{
		_visitEntity(root(), callback);
		forEachSystem([&callback](System &system) {
			if (auto *rendering = dynamic_cast<EngineView::Contributor *>(&system))
			{
				callback(*rendering);
			}
		});
	}

	EngineView *Engine::_findView(const std::string &name) const noexcept
	{
		const auto it = std::ranges::find_if(_views, [&name](const auto &view) {
			return view->name() == name;
		});
		return it == _views.end() ? nullptr : it->get();
	}

	EngineView &Engine::addView(std::string name, CameraComponent3D *camera, Vector2UInt resolution, std::optional<EngineView::Order> order)
	{
		std::lock_guard lock(_viewMutex);
		if (_findView(name) != nullptr)
		{
			throw spk::Exception("EngineView [" + name + "] already exists");
		}
		if (resolution.x == 0 || resolution.y == 0)
		{
			throw spk::Exception("EngineView [" + name + "]: resolution must be nonzero");
		}
		for (const auto &existing : _views)
		{
			if (order && existing->order() == order)
			{
				throw spk::Exception("EngineView [" + name + "]: duplicate view order " + std::to_string(*order));
			}
		}
		if (_nextViewIdentity == std::numeric_limits<std::uint64_t>::max())
		{
			throw spk::Exception("EngineView identity range exhausted");
		}
		auto result = std::unique_ptr<EngineView>(
			new EngineView(*this, std::move(name), _nextViewIdentity, camera, resolution, order));
		_views.push_back(std::move(result));
		++_nextViewIdentity;
		return *_views.back();
	}

	EngineView &Engine::view(const std::string &name)
	{
		std::lock_guard lock(_viewMutex);
		if (auto *result = _findView(name))
		{
			return *result;
		}
		throw spk::Exception("Unknown EngineView [" + name + "]");
	}

	const EngineView &Engine::view(const std::string &name) const
	{
		return const_cast<Engine *>(this)->view(name);
	}

	std::shared_ptr<EngineView::Output> Engine::viewOutput(const std::string &name) const
	{
		std::lock_guard lock(_viewMutex);
		if (const auto *result = _findView(name))
		{
			return result->output();
		}
		return {};
	}

	void Engine::removeView(const std::string &name)
	{
		std::unique_ptr<EngineView> removed;
		{
			std::lock_guard lock(_viewMutex);
			const auto it = std::ranges::find_if(_views, [&name](const auto &view) {
				return view->name() == name;
			});
			if (it == _views.end())
			{
				throw spk::Exception("Unknown EngineView [" + name + "]");
			}
			removed = std::move(*it);
			_views.erase(it);
		}
	}

	void Engine::setViewOrder(EngineView &view, std::optional<EngineView::Order> order)
	{
		std::lock_guard lock(_viewMutex);
		const bool owned = std::ranges::any_of(_views, [&view](const auto &item) {
			return item.get() == &view;
		});
		if (!owned)
		{
			throw spk::Exception("Cannot reorder a view not owned by this Engine");
		}
		for (const auto &item : _views)
		{
			if (item.get() != &view && order && item->order() == order)
			{
				throw spk::Exception("EngineView [" + view.name() + "]: duplicate view order " + std::to_string(*order));
			}
		}
		view._order = order;
	}

	std::vector<Engine::ViewCapture> Engine::_captureViews() const
	{
		std::lock_guard lock(_viewMutex);
		std::vector<ViewCapture> captures;
		for (const auto &view : _views)
		{
			if (view->camera() != nullptr)
			{
				captures.push_back({view->identity(), view->order(), view->_capture()});
			}
		}
		std::ranges::stable_sort(captures, [](const ViewCapture &a, const ViewCapture &b) {
			if (a.order && b.order)
			{
				return *a.order < *b.order;
			}
			return a.order.has_value() && !b.order.has_value();
		});
		return captures;
	}

	void Engine::_validateViewPasses(const std::string &name, const RenderSnapshot::Builder &builder)
	{
		const std::array systemKeys{EngineView::ShadowPassKey, EngineView::SkyboxPassKey, EngineView::OpaquePassKey, EngineView::WBOITPassKey, EngineView::CompositionPassKey};
		std::vector<RenderPass::Key> validated;
		for (const auto &key : builder.passKeys())
		{
			const auto system = std::ranges::find_if(systemKeys, [&key](const auto &expected) {
				return expected.name == key.name && expected.order == key.order;
			});
			if (system == systemKeys.end())
			{
				const auto reserved = std::ranges::any_of(systemKeys, [&key](const auto &expected) {
					return expected.name == key.name || expected.order == key.order;
				});
				if (key.name.empty() || key.name.starts_with("sparkle.") || reserved ||
					key.order > EngineView::CompositionPassKey.order)
				{
					throw spk::Exception("EngineView [" + name + "]: invalid pass [" + key.name + "] at order " + std::to_string(key.order) + " (expected custom ranges [-2147483648,-1000001], [-999999,-2], [1,999999], [1000001,1999999] or exact system key)");
				}
			}
			for (const auto &old : validated)
			{
				if (old.order == key.order && old.name != key.name)
				{
					throw spk::Exception("EngineView [" + name + "]: two passes at order " + std::to_string(key.order) + " [" + old.name + "] and [" + key.name + "]");
				}
			}
			validated.push_back(key);
		}
	}

	RenderSnapshot Engine::_buildViewSnapshot(const EngineView::CaptureContext &context)
	{
		RenderSnapshot::Builder builder;
		for (const auto &key : {EngineView::ShadowPassKey, EngineView::SkyboxPassKey, EngineView::OpaquePassKey, EngineView::WBOITPassKey, EngineView::CompositionPassKey})
		{
			builder.renderPass(key);
		}
		_visitObjects([&](EngineView::Contributor &contributor) {
			if (contributor.canContribute() && context.isLayerCompatible(contributor.layers()))
			{
				try
				{
					contributor.buildViewRenderSnapshot(builder, context);
				} catch (const std::exception &error)
				{
					throw spk::Exception("EngineView [" + context.name + "]: " + error.what(), std::current_exception());
				}
			}
		});
		_validateViewPasses(context.name, builder);
		return builder.build();
	}

	bool Engine::_commitSnapshot(const std::vector<ViewCapture> &views, RenderSnapshot snapshot)
	{
		std::lock_guard lock(_viewMutex);
		for (const auto &capture : views)
		{
			if (!std::ranges::any_of(_views, [&capture](const auto &view) {
					return view->identity() == capture.identity;
				}))
			{
				return false;
			}
		}
		_renderSnapshot.publish(std::move(snapshot));
		return true;
	}

	void Engine::refreshRenderSnapshot()
	{
		for (int attempt = 0; attempt < 16; ++attempt)
		{
			auto captures = _captureViews();
			RenderSnapshot::Builder builder;
			auto &pass = builder.renderPass({"sparkle.engine.views", 0});
			try
			{
				for (const auto &capture : captures)
				{
					auto snapshot = std::make_shared<const RenderSnapshot>(_buildViewSnapshot(capture.context));
					pass.emplace<ExecuteSnapshotRenderCommand>(std::move(snapshot));
				}
			} catch (...)
			{
				throw spk::Exception("Engine failed to build view snapshots", std::current_exception());
			}
			if (_commitSnapshot(captures, builder.build()))
			{
				return;
			}
		}
		throw spk::Exception("EngineView collection changed repeatedly during capture");
	}

	std::shared_ptr<const RenderSnapshot> Engine::latestRenderSnapshot() const noexcept
	{
		return _renderSnapshot.acquireLatest();
	}

	bool Engine::_isAcceptingEvent() const
	{
		return true;
	}

	void Engine::_propagateEvent(const std::function<void(EventDispatcher *)> &callback)
	{
		_visitObjects([&callback](EngineView::Contributor &object) {
			callback(&object);
		});
	}

	bool Engine::_canUpdateByDevice() const
	{
		return true;
	}

	void Engine::_afterUpdate(UpdateContext &context, DeviceContext &deviceContext)
	{
		_visitObjects([&](EngineView::Contributor &object) {
			object.updateState(context, deviceContext);
		});
	}

	bool Engine::_canBuildRenderSnapshot() const
	{
		return true;
	}

	void Engine::_afterBuildRenderSnapshot(RenderSnapshot::Builder &builder)
	{
		if (auto latest = _renderSnapshot.acquireLatest())
		{
			builder.renderPass(SceneRenderPassKey).emplace<ExecuteSnapshotRenderCommand>(std::move(latest));
		}
	}
}
