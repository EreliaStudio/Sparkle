#include "internal/application_internal.hpp"

#include <gl/gl.h>

#include <utility>
#include <variant>

#include "core/context/render_context.hpp"
#include "exception.hpp"

namespace spk
{
	Application::RenderRuntime::RenderRuntime(
		WinAPI::WakeEvent &wakeEvent,
		spk::ThreadSafeFIFO<RenderRequest>::Consumer renderRequestConsumer,
		spk::ThreadSafeFIFO<PlatformRequest>::Producer platformRequestProducer) :
		Runtime("render"),
		_platformRequestProducer(std::move(platformRequestProducer), wakeEvent),
		_renderRequestConsumer(std::move(renderRequestConsumer))
	{
	}

	void Application::RenderRuntime::_registerSnapshotConsumer(
		const Window::Identifier &identifier,
		spk::ThreadSafeSlot<spk::RenderSnapshot>::Consumer consumer,
		std::shared_ptr<std::atomic_bool> isRequested)
	{
		isRequested->store(true, std::memory_order_relaxed);
		auto [it, inserted] = _renderSnapshotEnties.emplace(identifier, RenderSnapshotEntry{std::move(consumer), std::move(isRequested)});
		if (!inserted)
		{
			throw std::logic_error("A render snapshot consumer already exists for [" + identifier + "]");
		}
	}

	void Application::RenderRuntime::_createSurface(Window::Surface &surface, const std::weak_ptr<Window::Native> &native)
	{
		const std::shared_ptr<Window::Native> lockedNative = native.lock();
		if (lockedNative == nullptr)
		{
			throw std::runtime_error("Cannot create the surface because its native frame was released");
		}
		surface.create(lockedNative->window());
	}

	void Application::RenderRuntime::_destroySurface(Window::Surface &surface)
	{
		surface.destroy();
	}

	void Application::RenderRuntime::_render(Window::Surface &surface, const spk::RenderSnapshot &snapshot)
	{
		const spk::Vector2UInt size = surface.geometry().size;
		if (size.x == 0 || size.y == 0)
		{
			return;
		}
		surface.makeCurrent();

		spk::RenderContext context{
			.targetSurface = &surface};

		snapshot.execute(context);

		surface.present();
	}

	void Application::RenderRuntime::_consume(const SurfaceRegistrationRequest &request)
	{
		append(request.windowIdentifier, request.surface);
		_registerSnapshotConsumer(request.windowIdentifier, request.renderSnapshotConsumer, request.isRequested);
	}

	void Application::RenderRuntime::_consume(const SurfaceCreationRequest &request)
	{
		Window::Surface *surface = tryGet(request.windowIdentifier);
		if (surface != nullptr)
		{
			_createSurface(*surface, request.native);
		}
	}

	void Application::RenderRuntime::_consume(const SurfaceResizeRequest &request)
	{
		Window::Surface *surface = tryGet(request.windowIdentifier);
		if (surface == nullptr)
		{
			return;
		}

		spk::Rect2D geometry = surface->geometry();
		geometry.size = {
			static_cast<spk::Rect2D::Size::value_type>(request.newSize.x),
			static_cast<spk::Rect2D::Size::value_type>(request.newSize.y)};

		surface->setGeometry(geometry);
	}

	void Application::RenderRuntime::_consume(const SurfaceDeletionRequest &request)
	{
		if (!contains(request.windowIdentifier))
		{
			return;
		}
		_destroySurface(object(request.windowIdentifier));
		remove(request.windowIdentifier);
		_renderSnapshotEnties.erase(request.windowIdentifier);
		_platformRequestProducer.publish(NativeDeletionRequest{.windowIdentifier = request.windowIdentifier});
	}

	void Application::RenderRuntime::_consumeRequests()
	{
		for (auto &request : _renderRequestConsumer.drain())
		{
			std::visit([this](const auto &value) {
				_consume(value);
			},
					   request);
		}
	}

	Application::RenderRuntime::RenderSnapshotEntry *Application::RenderRuntime::_tryGetSnapshotConsumer(
		const Window::Identifier &identifier)
	{
		auto it = _renderSnapshotEnties.find(identifier);
		return it != _renderSnapshotEnties.end() ? &it->second : nullptr;
	}

	void Application::RenderRuntime::consumeIncoming()
	{
		_consumeRequests();
	}

	void Application::RenderRuntime::tickOnce(const Window::Identifier &identifier, Window::Surface &surface)
	{
		if (surface.lifeCycle() != Window::LifeCycle::Ready)
		{
			return;
		}

		auto *entry = _tryGetSnapshotConsumer(identifier);
		if (entry == nullptr)
		{
			return;
		}

		auto snapshot = entry->consumer.acquireLatest();
		if (snapshot != nullptr && snapshot != entry->lastRenderedSnapshot)
		{
			entry->lastRenderedSnapshot = snapshot;
			try
			{
				_render(surface, *snapshot);
			} catch (spk::Exception &exception)
			{
				exception.addContext("Exception while rendering window [" + identifier + "]");
				throw;
			} catch (...)
			{
				throw spk::Exception(
					"Exception while rendering window [" + identifier + "]",
					std::current_exception());
			}
			entry->isRequested->store(true, std::memory_order_relaxed);
		}
		surface._gpuResources().reclaimReleased();
	}

	void Application::RenderRuntime::release(Window::Surface &surface)
	{
		_destroySurface(surface);
	}
}
