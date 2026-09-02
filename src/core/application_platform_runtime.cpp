#include "internal/application_internal.hpp"

#include <windowsx.h>

#include <string>
#include <system_error>
#include <utility>
#include <variant>

namespace spk
{
	Application::PlatformRuntime::PlatformRuntime(
		WinAPI::WakeEvent &wakeEvent,
		spk::ThreadSafeFIFO<PlatformRequest>::Consumer platformRequestConsumer,
		spk::ThreadSafeFIFO<EventRecord>::Producer eventRecordProducer,
		spk::ThreadSafeFIFO<UpdateRequest>::Producer updateRequestProducer,
		spk::ThreadSafeFIFO<RenderRequest>::Producer renderRequestProducer) :
		Runtime("platform"),
		_wakeEvent(wakeEvent),
		_windowClass(std::string(ClassIdentifier)),
		_platformRequestConsumer(std::move(platformRequestConsumer)),
		_eventRecordProducer(std::move(eventRecordProducer)),
		_updateRequestProducer(std::move(updateRequestProducer)),
		_renderRequestProducer(std::move(renderRequestProducer))
	{
	}

	void Application::PlatformRuntime::_createNative(const NativeRegistrationRequest &request)
	{
		const Identifier identifier = request.windowIdentifier;
		request.native->window().create(_windowClass, WinAPI::Window::CreationInfo{.title = request.configuration.title, .x = request.configuration.area.anchor.x, .y = request.configuration.area.anchor.y, .width = request.configuration.area.size.x, .height = request.configuration.area.size.y, .messageHandler = [this, identifier](HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
																					   return _processMessage(identifier, handle, message, wParam, lParam);
																				   }});
		request.native->markReady();
	}

	void Application::PlatformRuntime::_destroyNative(Window::Native &native)
	{
		if (native.lifeCycle() == Window::LifeCycle::Released)
		{
			return;
		}
		native.beginRelease();
		native.window().destroy();
		native.markReleased();
	}

	template <typename TRecord>
	void Application::PlatformRuntime::_publish(const Identifier &identifier, TRecord record)
	{
		record.windowIdentifier = identifier;
		_eventRecordProducer.publish(EventRecord(std::move(record)));
	}

	spk::Vector2Int Application::PlatformRuntime::_mousePosition(LPARAM lParam) noexcept
	{
		return {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
	}

	spk::Mouse::Button Application::PlatformRuntime::_mouseButton(UINT message) noexcept
	{
		switch (message)
		{
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
			return spk::Mouse::Right;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
			return spk::Mouse::Middle;
		default:
			return spk::Mouse::Left;
		}
	}

	spk::Keyboard::Key Application::PlatformRuntime::_key(WPARAM wParam, LPARAM lParam) noexcept
	{
		UINT key = static_cast<UINT>(wParam);
		if (key == VK_SHIFT)
		{
			key = ::MapVirtualKeyW((lParam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX);
		}
		else if (key == VK_CONTROL)
		{
			key = (lParam & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
		}
		else if (key == VK_MENU)
		{
			key = (lParam & (1 << 24)) ? VK_RMENU : VK_LMENU;
		}
		return key < spk::Keyboard::NbKey ? static_cast<spk::Keyboard::Key>(key) : spk::Keyboard::Unknown;
	}

	void Application::PlatformRuntime::_consume(const NativeRegistrationRequest &request)
	{
		append(request.windowIdentifier, request.native);
		_createNative(request);
		_renderRequestProducer.publish(SurfaceCreationRequest{.windowIdentifier = request.windowIdentifier, .native = request.native});
	}

	void Application::PlatformRuntime::_consume(const NativeDeletionRequest &request)
	{
		if (!contains(request.windowIdentifier))
		{
			return;
		}
		_mouseInsideWindows.erase(request.windowIdentifier);
		_destroyNative(object(request.windowIdentifier));
		remove(request.windowIdentifier);
	}

	void Application::PlatformRuntime::_consume(const MousePositionRequest &request)
	{
		if (!contains(request.windowIdentifier))
		{
			return;
		}

		POINT screenPosition{request.position.x, request.position.y};
		const HWND handle = object(request.windowIdentifier).window().handle();
		if (::ClientToScreen(handle, &screenPosition) == FALSE)
		{
			throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "ClientToScreen");
		}
		if (::SetCursorPos(screenPosition.x, screenPosition.y) == FALSE)
		{
			throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "SetCursorPos");
		}
	}

	void Application::PlatformRuntime::_consumeRequests()
	{
		for (auto &request : _platformRequestConsumer.drain())
		{
			std::visit([this](const auto &value) {
				_consume(value);
			},
					   request);
		}
	}

	Application::PlatformRuntime::MessageResult Application::PlatformRuntime::_processWindowMessage(
		const Identifier &identifier, UINT message, LPARAM lParam)
	{
		switch (message)
		{
		case WM_MOVE:
			_publish(identifier, WindowMovedRecord{});
			return 0;
		case WM_SIZE:
		{
			const spk::Vector2UInt newSize{
				static_cast<spk::Vector2UInt::value_type>(LOWORD(lParam)),
				static_cast<spk::Vector2UInt::value_type>(HIWORD(lParam))};

			_renderRequestProducer.publish(SurfaceResizeRequest{.windowIdentifier = identifier, .newSize = newSize});

			WindowResizedRecord record;
			record.size = newSize;
			_publish(identifier, std::move(record));
			return 0;
		}
		case WM_SETFOCUS:
			_publish(identifier, WindowFocusGainedRecord{});
			return 0;
		case WM_KILLFOCUS:
			_publish(identifier, WindowFocusLostRecord{});
			return 0;
		default:
			return std::nullopt;
		}
	}

	void Application::PlatformRuntime::_trackMouseLeave(HWND handle)
	{
		TRACKMOUSEEVENT event{.cbSize = sizeof(TRACKMOUSEEVENT), .dwFlags = TME_LEAVE, .hwndTrack = handle};
		if (::TrackMouseEvent(&event) == FALSE)
		{
			throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "TrackMouseEvent");
		}
	}

	void Application::PlatformRuntime::_processMouseMove(const Identifier &identifier, HWND handle, LPARAM lParam)
	{
		if (_mouseInsideWindows.insert(identifier).second)
		{
			_trackMouseLeave(handle);
			_publish(identifier, MouseEnteredRecord{});
		}
		MouseMovedRecord record;
		record.position = _mousePosition(lParam);
		_publish(identifier, std::move(record));
	}

	void Application::PlatformRuntime::_processMouseWheel(const Identifier &identifier, WPARAM wParam, bool horizontal)
	{
		const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
		MouseWheelScrolledRecord record;
		record.value = horizontal ? spk::Vector2(delta, 0.0f) : spk::Vector2(0.0f, delta);
		_publish(identifier, std::move(record));
	}

	template <typename TRecord>
	void Application::PlatformRuntime::_publishMouseButton(const Identifier &identifier, UINT message)
	{
		TRecord record;
		record.button = _mouseButton(message);
		_publish(identifier, std::move(record));
	}

	void Application::PlatformRuntime::_processMouseLeave(const Identifier &identifier)
	{
		_mouseInsideWindows.erase(identifier);
		_publish(identifier, MouseLeftRecord{});
	}

	Application::PlatformRuntime::MessageResult Application::PlatformRuntime::_processMouseButtonMessage(
		const Identifier &identifier, HWND handle, UINT message, WPARAM wParam)
	{
		switch (message)
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
			::SetCapture(handle);
			_publishMouseButton<MouseButtonPressedRecord>(identifier, message);
			return 0;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
			_publishMouseButton<MouseButtonReleasedRecord>(identifier, message);
			if ((wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) == 0)
			{
				::ReleaseCapture();
			}
			return 0;
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
			::SetCapture(handle);
			_publishMouseButton<MouseButtonDoubleClickedRecord>(identifier, message);
			return 0;
		default:
			return std::nullopt;
		}
	}

	Application::PlatformRuntime::MessageResult Application::PlatformRuntime::_processMouseMessage(
		const Identifier &identifier, HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_MOUSEMOVE)
		{
			_processMouseMove(identifier, handle, lParam);
			return 0;
		}
		if (message == WM_MOUSELEAVE)
		{
			_processMouseLeave(identifier);
			return 0;
		}
		if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
		{
			_processMouseWheel(identifier, wParam, message == WM_MOUSEHWHEEL);
			return 0;
		}
		return _processMouseButtonMessage(identifier, handle, message, wParam);
	}

	template <typename TRecord>
	void Application::PlatformRuntime::_publishKey(const Identifier &identifier, WPARAM wParam, LPARAM lParam)
	{
		TRecord record;
		record.key = _key(wParam, lParam);
		_publish(identifier, std::move(record));
	}

	Application::PlatformRuntime::MessageResult Application::PlatformRuntime::_processKeyboardMessage(
		const Identifier &identifier, HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_KEYDOWN:
			_publishKey<KeyPressedRecord>(identifier, wParam, lParam);
			return 0;
		case WM_KEYUP:
			_publishKey<KeyReleasedRecord>(identifier, wParam, lParam);
			return 0;
		case WM_SYSKEYDOWN:
			_publishKey<KeyPressedRecord>(identifier, wParam, lParam);
			return ::DefWindowProcW(handle, message, wParam, lParam);
		case WM_SYSKEYUP:
			_publishKey<KeyReleasedRecord>(identifier, wParam, lParam);
			return ::DefWindowProcW(handle, message, wParam, lParam);
		case WM_CHAR:
		{
			TextInputRecord record;
			record.glyph = static_cast<wchar_t>(wParam);
			_publish(identifier, std::move(record));
			return 0;
		}
		default:
			return std::nullopt;
		}
	}

	LRESULT Application::PlatformRuntime::_processMessage(
		const Identifier &identifier, HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (auto result = _processWindowMessage(identifier, message, lParam))
		{
			return *result;
		}
		if (auto result = _processMouseMessage(identifier, handle, message, wParam, lParam))
		{
			return *result;
		}
		if (auto result = _processKeyboardMessage(identifier, handle, message, wParam, lParam))
		{
			return *result;
		}
		return ::DefWindowProcW(handle, message, wParam, lParam);
	}

	void Application::PlatformRuntime::_pullEvents()
	{
		WinAPI::MessageQueue::dispatchPending();
	}
	void Application::PlatformRuntime::consumeIncoming()
	{
		_consumeRequests();
	}
	void Application::PlatformRuntime::prepareCycle()
	{
		_pullEvents();
	}

	void Application::PlatformRuntime::tickOnce(const Window::Identifier &identifier, Window::Native &native)
	{
		native.window().rethrowPendingException();
		if (!native.window().consumeClosureRequest())
		{
			return;
		}
		native.beginRelease();
		_updateRequestProducer.publish(StateDeletionRequest{.windowIdentifier = identifier});
		_renderRequestProducer.publish(SurfaceDeletionRequest{.windowIdentifier = identifier});
	}

	void Application::PlatformRuntime::release(Window::Native &native)
	{
		_destroyNative(native);
	}
}
