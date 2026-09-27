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
			std::move(channels.renderRequests.producer),
			[this](const Window::Identifier &identifier) {
				static_cast<void>(_requestWindowClosure(identifier));
			}),
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
		std::unique_lock lock(_windowClosureMutex);
		if (const auto found = _windowClosureOperations.find(identifier); found != _windowClosureOperations.end())
		{
			return found->second->task.answer();
		}

		static_cast<void>(window(identifier));
		auto operation = std::make_unique<WindowClosureOperation>();
		Task<void>::Answer answer = operation->task.answer();
		WindowClosureOperation &stored = *operation;
		_windowClosureOperations.emplace(identifier, std::move(operation));


		stored.surfaceContract.emplace(
			stored.surfaceDeletion->answer().subscribeToCompletion(
				[this, identifier] {
					_completeWindowClosure(identifier);
				}));
		stored.stateContract.emplace(
			stored.stateDeletion->answer().subscribeToCompletion(
				[this, identifier] {
					_completeWindowClosure(identifier);
				}));
		stored.nativeContract.emplace(
			stored.nativeDeletion->answer().subscribeToCompletion(
				[this, identifier] {
					_completeWindowClosure(identifier);
				}));

		auto stateDeletion = stored.stateDeletion;
		auto surfaceDeletion = stored.surfaceDeletion;
		lock.unlock();

		_updateRequestProducer.publish(
			StateDeletionRequest{
				.windowIdentifier = identifier,
				.task = std::move(stateDeletion)});
		_renderRequestProducer.publish(
			SurfaceDeletionRequest{.windowIdentifier = identifier, .task = std::move(surfaceDeletion), .nativeDeletionTask = stored.nativeDeletion});
		return answer;
	}

	void Application::Impl::_requestAllWindowClosures()
	{
		for (const auto &[identifier, window] : _windows)
		{
			static_cast<void>(_requestWindowClosure(identifier));
		}
	}

	void Application::Impl::_completeWindowClosure(const Window::Identifier &identifier)
	{
		bool shouldWake = false;
		{
			const std::scoped_lock lock(_windowClosureMutex);
			const auto found = _windowClosureOperations.find(identifier);
			if (found == _windowClosureOperations.end())
			{
				return;
			}

			const Task<void>::Status stateStatus = found->second->stateDeletion->answer().status();
			const Task<void>::Status surfaceStatus = found->second->surfaceDeletion->answer().status();
			const Task<void>::Status nativeStatus = found->second->nativeDeletion->answer().status();
			const bool stateSettled = stateStatus != Task<void>::Status::Pending;
			const bool surfaceSettled = surfaceStatus != Task<void>::Status::Pending;
			const bool nativeRequired = surfaceStatus == Task<void>::Status::Completed;
			const bool nativeSettled = nativeStatus != Task<void>::Status::Pending;

			if (stateSettled == false || surfaceSettled == false || (nativeRequired == true && nativeSettled == false))
			{
				return;
			}

			if (std::find(_completedWindowClosures.begin(), _completedWindowClosures.end(), identifier) == _completedWindowClosures.end())
			{
				_completedWindowClosures.push_back(identifier);
				shouldWake = true;
			}
		}
		if (shouldWake == true)
		{
			_platformWakeEvent.notify();
		}
	}

	void Application::Impl::_removeCompletedWindows()
	{
		std::vector<Window::Identifier> completed;
		{
			const std::scoped_lock lock(_windowClosureMutex);
			completed.swap(_completedWindowClosures);
		}

		for (const Window::Identifier &identifier : completed)
		{
			std::unique_ptr<WindowClosureOperation> operation;
			{
				const std::scoped_lock lock(_windowClosureMutex);
				const auto found = _windowClosureOperations.find(identifier);
				if (found == _windowClosureOperations.end())
				{
					continue;
				}
				operation = std::move(found->second);
				_windowClosureOperations.erase(found);
			}

			const auto failure = [&]() -> std::exception_ptr {
				for (const auto &task : {operation->stateDeletion, operation->surfaceDeletion, operation->nativeDeletion})
				{
					const auto taskAnswer = task->answer();
					if (taskAnswer.status() == Task<void>::Status::Failed)
					{
						return taskAnswer.failure();
					}
				}
				return nullptr;
			}();

			_windows.erase(identifier);
			if (failure != nullptr)
			{
				operation->task.fail(failure);
			}
			else
			{
				operation->task.validate();
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
			_removeCompletedWindows();
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
		_platformWakeEvent.notify();
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
