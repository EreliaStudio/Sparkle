#pragma once

#include "core/application.hpp"
#include "exception.hpp"

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <stop_token>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "container/thread_safe_fifo.hpp"
#include "container/thread_safe_slot.hpp"
#include "core/context/update_context.hpp"
#include "core/event/platform_request.hpp"
#include "core/event/record.hpp"
#include "core/event/render_request.hpp"
#include "core/event/update_request.hpp"
#include "core/platform/message_queue.hpp"
#include "core/platform/wake_event.hpp"
#include "core/platform/window.hpp"
#include "core/window.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk
{
	class PlatformRequestProducer
	{
	private:
		spk::ThreadSafeFIFO<PlatformRequest>::Producer _producer;
		WinAPI::WakeEvent &_wakeEvent;

	public:
		PlatformRequestProducer(
			spk::ThreadSafeFIFO<PlatformRequest>::Producer producer,
			WinAPI::WakeEvent &wakeEvent) :
			_producer(std::move(producer)),
			_wakeEvent(wakeEvent)
		{
		}

		void publish(PlatformRequest request)
		{
			_producer.publish(std::move(request));
			_wakeEvent.notify();
		}
	};

	struct Application::Channels
	{
		spk::ThreadSafeFIFO<EventRecord>::Endpoints eventRecords;
		spk::ThreadSafeFIFO<PlatformRequest>::Endpoints platformRequests;
		spk::ThreadSafeFIFO<UpdateRequest>::Endpoints updateRequests;
		spk::ThreadSafeFIFO<RenderRequest>::Endpoints renderRequests;

		Channels();
	};

	template <typename TType>
	class Application::Runtime
	{
	protected:
		using Identifier = Window::Identifier;
		using Pointer = std::shared_ptr<TType>;
		using Collection = std::unordered_map<Identifier, Pointer>;

	private:
		Collection _registeredObjects;
		std::string_view _name;

	protected:
		void append(const Identifier &identifier, Pointer object)
		{
			auto [it, inserted] = _registeredObjects.emplace(identifier, std::move(object));
			if (!inserted)
			{
				throw std::logic_error("An object is already registered for window [" + identifier + "]");
			}
		}

		void remove(const Identifier &identifier)
		{
			_registeredObjects.erase(identifier);
		}
		[[nodiscard]] bool contains(const Identifier &identifier) const
		{
			return _registeredObjects.contains(identifier);
		}

		[[nodiscard]] TType *tryGet(const Identifier &identifier)
		{
			auto it = _registeredObjects.find(identifier);
			return it != _registeredObjects.end() ? it->second.get() : nullptr;
		}

		[[nodiscard]] TType &object(const Identifier &identifier)
		{
			return *_registeredObjects.at(identifier);
		}
		virtual void consumeIncoming() = 0;
		virtual void prepareCycle()
		{
		}
		virtual void tickOnce(const Identifier &identifier, TType &object) = 0;
		virtual void finishCycle()
		{
		}
		virtual void release(TType &)
		{
		}

	public:
		explicit Runtime(std::string_view name) :
			_name(name)
		{
		}

		virtual ~Runtime() = default;

		void executeOnce()
		{
			try
			{
				consumeIncoming();
				prepareCycle();
			} catch (spk::Exception &exception)
			{
				exception.addContext("Exception while preparing " + std::string(_name) + " runtime");
				throw;
			} catch (...)
			{
				throw spk::Exception(
					"Exception while preparing " + std::string(_name) + " runtime",
					std::current_exception());
			}
			for (auto &[identifier, object] : _registeredObjects)
			{
				try
				{
					tickOnce(identifier, *object);
				} catch (spk::Exception &exception)
				{
					exception.addContext(
						"Exception in " + std::string(_name) + " runtime for window [" + identifier + "]");
					throw;
				} catch (...)
				{
					throw spk::Exception(
						"Exception in " + std::string(_name) + " runtime for window [" + identifier + "]",
						std::current_exception());
				}
			}
			try
			{
				finishCycle();
			} catch (spk::Exception &exception)
			{
				exception.addContext("Exception while finishing " + std::string(_name) + " runtime cycle");
				throw;
			} catch (...)
			{
				throw spk::Exception(
					"Exception while finishing " + std::string(_name) + " runtime cycle",
					std::current_exception());
			}
		}

		void shutdown()
		{
			for (auto &[identifier, object] : _registeredObjects)
			{
				try
				{
					release(*object);
				} catch (spk::Exception &exception)
				{
					exception.addContext(
						"Exception while releasing window [" + identifier + "] from " + std::string(_name) + " runtime");
					throw;
				} catch (...)
				{
					throw spk::Exception(
						"Exception while releasing window [" + identifier + "] from " + std::string(_name) + " runtime",
						std::current_exception());
				}
			}
			_registeredObjects.clear();
		}
	};

	class Application::PlatformRuntime final : public Runtime<Window::Native>
	{
	private:
		using MessageResult = std::optional<LRESULT>;
		static constexpr std::string_view ClassIdentifier = "sparkle.class";

		WinAPI::Window::Class _windowClass;
		WinAPI::WakeEvent &_wakeEvent;
		std::unordered_set<Window::Identifier> _mouseInsideWindows;
		spk::ThreadSafeFIFO<PlatformRequest>::Consumer _platformRequestConsumer;
		spk::ThreadSafeFIFO<EventRecord>::Producer _eventRecordProducer;
		spk::ThreadSafeFIFO<UpdateRequest>::Producer _updateRequestProducer;
		spk::ThreadSafeFIFO<RenderRequest>::Producer _renderRequestProducer;

		void _createNative(const NativeRegistrationRequest &request);
		void _destroyNative(Window::Native &native);

		template <typename TRecord>
		void _publish(const Identifier &identifier, TRecord record);

		[[nodiscard]] static spk::Vector2Int _mousePosition(LPARAM lParam) noexcept;
		[[nodiscard]] static spk::Mouse::Button _mouseButton(UINT message) noexcept;
		[[nodiscard]] static spk::Keyboard::Key _key(WPARAM wParam, LPARAM lParam) noexcept;
		void _consume(const NativeRegistrationRequest &request);
		void _consume(const NativeDeletionRequest &request);
		void _consumeRequests();
		[[nodiscard]] MessageResult _processWindowMessage(const Identifier &identifier, UINT message, LPARAM lParam);
		void _trackMouseLeave(HWND handle);
		void _processMouseMove(const Identifier &identifier, HWND handle, LPARAM lParam);
		void _processMouseWheel(const Identifier &identifier, WPARAM wParam, bool horizontal);

		template <typename TRecord>
		void _publishMouseButton(const Identifier &identifier, UINT message);

		void _processMouseLeave(const Identifier &identifier);
		[[nodiscard]] MessageResult _processMouseButtonMessage(const Identifier &identifier, HWND handle, UINT message, WPARAM wParam);
		[[nodiscard]] MessageResult _processMouseMessage(const Identifier &identifier, HWND handle, UINT message, WPARAM wParam, LPARAM lParam);

		template <typename TRecord>
		void _publishKey(const Identifier &identifier, WPARAM wParam, LPARAM lParam);

		[[nodiscard]] MessageResult _processKeyboardMessage(const Identifier &identifier, HWND handle, UINT message, WPARAM wParam, LPARAM lParam);
		LRESULT _processMessage(const Identifier &identifier, HWND handle, UINT message, WPARAM wParam, LPARAM lParam);
		void _pullEvents();

	protected:
		void consumeIncoming() override;
		void prepareCycle() override;
		void tickOnce(const Window::Identifier &identifier, Window::Native &native) override;
		void release(Window::Native &native) override;

	public:
		PlatformRuntime(
			WinAPI::WakeEvent &wakeEvent,
			spk::ThreadSafeFIFO<PlatformRequest>::Consumer platformRequestConsumer,
			spk::ThreadSafeFIFO<EventRecord>::Producer eventRecordProducer,
			spk::ThreadSafeFIFO<UpdateRequest>::Producer updateRequestProducer,
			spk::ThreadSafeFIFO<RenderRequest>::Producer renderRequestProducer);

		void waitForActivity()
		{
			WinAPI::MessageQueue::waitForActivity(_wakeEvent.handle());
		}
	};

	class Application::UpdateRuntime final : public Runtime<Window::State>
	{
	private:
		struct RenderSnapshotEntry
		{
			spk::ThreadSafeSlot<spk::RenderSnapshot>::Producer producer;
			std::shared_ptr<std::atomic_bool> isRequested;
		};

		spk::ThreadSafeFIFO<EventRecord>::Consumer _eventRecordConsumer;
		spk::ThreadSafeFIFO<UpdateRequest>::Consumer _updateRequestConsumer;
		std::unordered_map<Window::Identifier, RenderSnapshotEntry> _renderSnapshotEntries;
		std::chrono::steady_clock::time_point _startTime;
		std::chrono::steady_clock::time_point _currentTime;
		std::optional<std::chrono::steady_clock::time_point> _lastTime;
		std::chrono::steady_clock::duration _deltaTime = std::chrono::steady_clock::duration::zero();

		void _registerSnapshotProducer(
			const Window::Identifier &identifier,
			spk::ThreadSafeSlot<spk::RenderSnapshot>::Producer producer,
			std::shared_ptr<std::atomic_bool> isRequested);

		template <typename TEvent>
		void _applyFocusChanges(const TEvent &event, Window::State &state);

		template <typename TRecord>
		void _dispatch(const TRecord &record, Window::State &state);

		template <typename TRecord>
		void _dispatchMouse(const TRecord &record, Window::State &state);

		template <typename TRecord>
		void _dispatchKeyboard(const TRecord &record, Window::State &state);

		template <typename TRecord>
		[[nodiscard]] Window::State *_state(const TRecord &record);

		void _consume(const WindowResizedRecord &record);
		void _consume(const WindowMovedRecord &record);
		void _consume(const WindowFocusGainedRecord &record);
		void _consume(const WindowFocusLostRecord &record);
		void _consume(const MouseEnteredRecord &record);
		void _consume(const MouseLeftRecord &record);
		void _consume(const MouseMovedRecord &record);
		void _consume(const MouseWheelScrolledRecord &record);
		void _consume(const MouseButtonPressedRecord &record);
		void _consume(const MouseButtonReleasedRecord &record);
		void _consume(const MouseButtonDoubleClickedRecord &record);
		void _consume(const KeyPressedRecord &record);
		void _consume(const KeyReleasedRecord &record);
		void _consume(const TextInputRecord &record);
		void _consume(const EventRecord &event);
		void _consume(const StateRegistrationRequest &request);
		void _consume(const StateDeletionRequest &request);
		void _consumeEvents();
		void _consumeRequests();
		void _resetInput(Window::State &state);
		void _updateState(Window::State &state, UpdateContext &context);
		[[nodiscard]] spk::RenderSnapshot _buildRenderSnapshot(const Window::Identifier &identifier, Window::State &state);
		void _publishSnapshot(const Window::Identifier &identifier, spk::RenderSnapshot &&snapshot);
		bool _consumeSnapshotRequest(const Window::Identifier &identifier);

	protected:
		void consumeIncoming() override;
		void prepareCycle() override;
		void tickOnce(const Window::Identifier &identifier, Window::State &state) override;
		void finishCycle() override;
		void release(Window::State &state) override;

	public:
		UpdateRuntime(
			spk::ThreadSafeFIFO<EventRecord>::Consumer eventRecordConsumer,
			spk::ThreadSafeFIFO<UpdateRequest>::Consumer updateRequestConsumer);
	};

	class Application::RenderRuntime final : public Runtime<Window::Surface>
	{
	private:
		struct RenderSnapshotEntry
		{
			spk::ThreadSafeSlot<spk::RenderSnapshot>::Consumer consumer;
			std::shared_ptr<std::atomic_bool> isRequested;
			spk::ThreadSafeSlot<spk::RenderSnapshot>::pointer lastRenderedSnapshot;
		};

		PlatformRequestProducer _platformRequestProducer;
		spk::ThreadSafeFIFO<RenderRequest>::Consumer _renderRequestConsumer;
		std::unordered_map<Window::Identifier, RenderSnapshotEntry> _renderSnapshotEnties;

		void _registerSnapshotConsumer(
			const Window::Identifier &identifier,
			spk::ThreadSafeSlot<spk::RenderSnapshot>::Consumer consumer,
			std::shared_ptr<std::atomic_bool> isRequested);
		void _createSurface(Window::Surface &surface, const std::weak_ptr<Window::Native> &native);
		void _destroySurface(Window::Surface &surface);
		void _render(Window::Surface &surface, const spk::RenderSnapshot &snapshot);
		void _consume(const SurfaceRegistrationRequest &request);
		void _consume(const SurfaceCreationRequest &request);
		void _consume(const SurfaceResizeRequest &request);
		void _consume(const SurfaceDeletionRequest &request);
		void _consumeRequests();
		[[nodiscard]] RenderSnapshotEntry *_tryGetSnapshotConsumer(const Window::Identifier &identifier);

	protected:
		void consumeIncoming() override;
		void tickOnce(const Window::Identifier &identifier, Window::Surface &surface) override;
		void release(Window::Surface &surface) override;

	public:
		RenderRuntime(
			WinAPI::WakeEvent &wakeEvent,
			spk::ThreadSafeFIFO<RenderRequest>::Consumer renderRequestConsumer,
			spk::ThreadSafeFIFO<PlatformRequest>::Producer platformRequestProducer);
	};

	class Application::Impl
	{
	private:
		WinAPI::WakeEvent _platformWakeEvent;
		std::unordered_map<Window::Identifier, std::unique_ptr<Window>> _windows;
		PlatformRequestProducer _platformRequestProducer;
		spk::ThreadSafeFIFO<UpdateRequest>::Producer _updateRequestProducer;
		spk::ThreadSafeFIFO<RenderRequest>::Producer _renderRequestProducer;
		PlatformRuntime _platform;
		UpdateRuntime _updater;
		RenderRuntime _renderer;
		std::stop_source _stopSource;
		std::atomic<std::optional<int>> _exitCode{std::nullopt};
		std::mutex _workerExceptionMutex;
		std::exception_ptr _workerException = nullptr;

		explicit Impl(Channels channels);

		template <typename TRuntime>
		void _shutdownWorker(TRuntime &runtime);

		template <typename TRuntime>
		void _runWorker(TRuntime &runtime, std::stop_token stopToken);

		template <typename TRuntime>
		[[nodiscard]] std::jthread _startWorker(TRuntime &runtime);

		void _reportWorkerFailure(std::exception_ptr exception);
		void _rethrowWorkerFailure();
		void _stopAndJoinWorkers(std::jthread &updaterThread, std::jthread &rendererThread);
		void _registerWindowObjects(const Window::Identifier &identifier, const Window::Configuration &configuration, std::shared_ptr<Window::Native> native, std::shared_ptr<Window::State> state, std::shared_ptr<Window::Surface> surface, spk::ThreadSafeSlot<spk::RenderSnapshot>::Endpoints channel, std::shared_ptr<std::atomic_bool> isRenderSnapshotRequested);
		void _requestWindowClosure(const Window::Identifier &identifier);
		void _requestAllWindowClosures();
		void _removeClosedWindows();
		void _finishExecution();
		void _processApplicationState(bool &closureRequested);
		void _runPlatform();
		void _shutdownAfterFailure() noexcept;

	public:
		Impl();
		[[nodiscard]] Window &window(const Window::Identifier &identifier);
		[[nodiscard]] const Window &window(const Window::Identifier &identifier) const;
		Window &createWindow(const Window::Identifier &identifier, const Window::Configuration &configuration);
		void closeWindow(const Window::Identifier &identifier);
		void quit(int exitCode);
		int run();
	};
}
