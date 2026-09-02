#include "internal/application_internal.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace spk
{
	Application::Channels::Channels() :
		eventRecords(spk::ThreadSafeFIFO<EventRecord>::create()),
		platformRequests(spk::ThreadSafeFIFO<PlatformRequest>::create()),
		updateRequests(spk::ThreadSafeFIFO<UpdateRequest>::create()),
		renderRequests(spk::ThreadSafeFIFO<RenderRequest>::create())
	{
	}

	Application::Impl::Impl() :
		Impl(Channels{})
	{
	}

	Application::Impl::Impl(Channels channels) :
		_platformRequestProducer(channels.platformRequests.producer, _platformWakeEvent),
		_updateRequestProducer(channels.updateRequests.producer),
		_renderRequestProducer(channels.renderRequests.producer),
		_platform(
			_platformWakeEvent,
			std::move(channels.platformRequests.consumer),
			std::move(channels.eventRecords.producer),
			std::move(channels.updateRequests.producer),
			std::move(channels.renderRequests.producer)),
		_updater(
			_platformWakeEvent,
			channels.platformRequests.producer,
			std::move(channels.eventRecords.consumer),
			std::move(channels.updateRequests.consumer)),
		_renderer(_platformWakeEvent, std::move(channels.renderRequests.consumer), std::move(channels.platformRequests.producer))
	{
	}

	template <typename TRuntime>
	void Application::Impl::_shutdownWorker(TRuntime &runtime)
	{
		try
		{
			runtime.shutdown();
		} catch (...)
		{
			_reportWorkerFailure(std::current_exception());
		}
	}

	template <typename TRuntime>
	void Application::Impl::_runWorker(TRuntime &runtime, std::stop_token stopToken)
	{
		try
		{
			while (!stopToken.stop_requested() && !_stopSource.stop_requested())
			{
				runtime.executeOnce();
			}
		} catch (...)
		{
			_reportWorkerFailure(std::current_exception());
		}
		_shutdownWorker(runtime);
	}

	template <typename TRuntime>
	std::jthread Application::Impl::_startWorker(TRuntime &runtime)
	{
		return std::jthread([this, &runtime](std::stop_token stopToken) {
			_runWorker(runtime, stopToken);
		});
	}

	void Application::Impl::_reportWorkerFailure(std::exception_ptr exception)
	{
		{
			const std::scoped_lock lock(_workerExceptionMutex);
			if (_workerException == nullptr)
			{
				_workerException = std::move(exception);
			}
		}
		_exitCode.store(EXIT_FAILURE);
		_stopSource.request_stop();
	}

	void Application::Impl::_rethrowWorkerFailure()
	{
		std::exception_ptr exception;
		{
			const std::scoped_lock lock(_workerExceptionMutex);
			exception = _workerException;
		}
		if (exception != nullptr)
		{
			std::rethrow_exception(exception);
		}
	}

	void Application::Impl::_stopAndJoinWorkers(std::jthread &updaterThread, std::jthread &rendererThread)
	{
		_stopSource.request_stop();
		updaterThread.request_stop();
		rendererThread.request_stop();
		if (updaterThread.joinable())
		{
			updaterThread.join();
		}
		if (rendererThread.joinable())
		{
			rendererThread.join();
		}
	}

	void Application::Impl::_registerWindowObjects(
		const Window::Identifier &identifier, const Window::Configuration &configuration, std::shared_ptr<Window::Native> native, std::shared_ptr<Window::State> state, std::shared_ptr<Window::Surface> surface, spk::ThreadSafeSlot<spk::RenderSnapshot>::Endpoints channel, std::shared_ptr<std::atomic_bool> isRenderSnapshotRequested)
	{
		_updateRequestProducer.publish(StateRegistrationRequest{.windowIdentifier = identifier, .backgroundColor = configuration.backgroundColor, .state = std::move(state), .renderSnapshotProducer = std::move(channel.producer), .isRequested = isRenderSnapshotRequested});
		_renderRequestProducer.publish(SurfaceRegistrationRequest{.windowIdentifier = identifier, .surface = std::move(surface), .renderSnapshotConsumer = std::move(channel.consumer), .isRequested = isRenderSnapshotRequested});
		_platformRequestProducer.publish(NativeRegistrationRequest{.windowIdentifier = identifier, .configuration = configuration, .native = std::move(native)});
	}

	void Application::Impl::_requestWindowClosure(const Window::Identifier &identifier)
	{
		_updateRequestProducer.publish(StateDeletionRequest{.windowIdentifier = identifier});
		_renderRequestProducer.publish(SurfaceDeletionRequest{.windowIdentifier = identifier});
	}

	void Application::Impl::_requestAllWindowClosures()
	{
		for (const auto &[identifier, window] : _windows)
		{
			_requestWindowClosure(identifier);
		}
	}

	void Application::Impl::_removeClosedWindows()
	{
		std::erase_if(_windows, [](const auto &entry) {
			return entry.second->isClosed();
		});
	}

	void Application::Impl::_finishExecution()
	{
		if (!_exitCode.load().has_value())
		{
			_exitCode.store(EXIT_SUCCESS);
		}
		_stopSource.request_stop();
	}

	void Application::Impl::_processApplicationState(bool &closureRequested)
	{
		if (_windows.empty())
		{
			_finishExecution();
			return;
		}
		if (_exitCode.load().has_value() && !closureRequested)
		{
			closureRequested = true;
			_requestAllWindowClosures();
		}
	}

	void Application::Impl::_runPlatform()
	{
		bool closureRequested = false;
		std::stop_callback stopCallback(_stopSource.get_token(), [this] {
			_platformWakeEvent.notify();
		});

		while (!_stopSource.stop_requested())
		{
			_platform.executeOnce();
			_removeClosedWindows();
			_processApplicationState(closureRequested);
			if (!_stopSource.stop_requested())
			{
				_platform.waitForActivity();
			}
		}
	}

	void Application::Impl::_shutdownAfterFailure() noexcept
	{
		try
		{
			_platform.shutdown();
		} catch (...)
		{
		}
		_windows.clear();
	}

	Window &Application::Impl::window(const Window::Identifier &identifier)
	{
		return *_windows.at(identifier);
	}

	const Window &Application::Impl::window(const Window::Identifier &identifier) const
	{
		return *_windows.at(identifier);
	}

	Window &Application::Impl::createWindow(const Window::Identifier &identifier, const Window::Configuration &configuration)
	{
		if (_windows.contains(identifier))
		{
			throw std::logic_error("A window already exists with identifier [" + identifier + "]");
		}

		auto native = std::make_shared<Window::Native>(identifier);
		auto state = std::make_shared<Window::State>(identifier);
		auto surface = std::make_shared<Window::Surface>(identifier);
		auto window = std::make_unique<Window>(native, state, surface);

		Window &result = *window;
		_windows.emplace(identifier, std::move(window));
		_registerWindowObjects(identifier, configuration, std::move(native), std::move(state), std::move(surface), spk::ThreadSafeSlot<spk::RenderSnapshot>::create(), std::make_shared<std::atomic_bool>(true));
		return result;
	}

	void Application::Impl::closeWindow(const Window::Identifier &identifier)
	{
		static_cast<void>(window(identifier));
		_requestWindowClosure(identifier);
	}

	void Application::Impl::quit(int exitCode)
	{
		_exitCode.store(exitCode);
	}

	int Application::Impl::run()
	{
		std::jthread updaterThread;
		std::jthread rendererThread;
		try
		{
			updaterThread = _startWorker(_updater);
			rendererThread = _startWorker(_renderer);
			_runPlatform();
			_stopAndJoinWorkers(updaterThread, rendererThread);
			_rethrowWorkerFailure();
			return _exitCode.load().value_or(EXIT_SUCCESS);
		} catch (...)
		{
			auto exception = std::current_exception();
			_stopAndJoinWorkers(updaterThread, rendererThread);
			_shutdownAfterFailure();
			std::rethrow_exception(exception);
		}
	}
}
