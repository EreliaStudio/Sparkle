#include "internal/application_internal.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>

#include "diagnostics/logger.hpp"
#include "ui/widget.hpp"

namespace
{
	void logApplicationException(const std::exception_ptr &exception) noexcept
	{
		try
		{
			std::rethrow_exception(exception);
		} catch (const std::exception &caught)
		{
			spk::logger << spk::Logger::setLevel(spk::Logger::Level::Error) << caught.what() << std::endl;
		} catch (...)
		{
			spk::logger << spk::Logger::setLevel(spk::Logger::Level::Error) << "Unknown exception" << std::endl;
		}
	}
}

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

	Task<void>::Answer Application::Impl::_requestWindowClosure(const Window::Identifier &identifier)
	{
		const std::scoped_lock lock(_windowClosureMutex);
		if (const auto found = _windowClosureTasks.find(identifier); found != _windowClosureTasks.end())
		{
			return found->second.answer();
		}

		static_cast<void>(window(identifier));
		auto [task, inserted] = _windowClosureTasks.emplace(identifier, Task<void>{});
		static_cast<void>(inserted);
		Task<void>::Answer answer = task->second.answer();

		_updateRequestProducer.publish(StateDeletionRequest{.windowIdentifier = identifier});
		_renderRequestProducer.publish(SurfaceDeletionRequest{.windowIdentifier = identifier});
		return answer;
	}

	void Application::Impl::_requestAllWindowClosures()
	{
		for (const auto &[identifier, window] : _windows)
		{
			static_cast<void>(_requestWindowClosure(identifier));
		}
	}

	void Application::Impl::_removeClosedWindows()
	{
		for (auto iterator = _windows.begin(); iterator != _windows.end();)
		{
			if (iterator->second->isClosed() == false)
			{
				++iterator;
				continue;
			}

			const Window::Identifier identifier = iterator->first;
			iterator = _windows.erase(iterator);

			const std::scoped_lock lock(_windowClosureMutex);
			if (const auto found = _windowClosureTasks.find(identifier); found != _windowClosureTasks.end())
			{
				found->second.validate();
				_windowClosureTasks.erase(found);
			}
		}
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
		state->root().setGeometry(spk::Rect2D{
			.anchor = {0, 0},
			.size = configuration.area.size});
		auto surface = std::make_shared<Window::Surface>(identifier);
		auto window = std::make_unique<Window>(native, state, surface);

		Window &result = *window;
		_windows.emplace(identifier, std::move(window));
		_registerWindowObjects(identifier, configuration, std::move(native), std::move(state), std::move(surface), spk::ThreadSafeSlot<spk::RenderSnapshot>::create(), std::make_shared<std::atomic_bool>(true));
		return result;
	}

	Task<void>::Answer Application::Impl::closeWindow(const Window::Identifier &identifier)
	{
		return _requestWindowClosure(identifier);
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
			logApplicationException(exception);
			std::rethrow_exception(exception);
		}
	}
}
