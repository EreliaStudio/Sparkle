#include "core/platform/window.hpp"

#include <stdexcept>
#include <system_error>
#include <utility>

namespace spk::WinAPI
{
	Window::Class::Class(std::string identifier) :
		_instance(::GetModuleHandleW(nullptr)),
		_identifier(Window::_toWide(identifier))
	{
		if (_instance == nullptr)
		{
			Window::_throwLastError("GetModuleHandleW");
		}
		_register();
	}

	Window::Class::~Class()
	{
		if (_atom != 0)
		{
			::UnregisterClassW(_identifier.c_str(), _instance);
		}
	}

	void Window::Class::_register()
	{
		WNDCLASSEXW description{};
		description.cbSize = sizeof(description);
		description.style = CS_OWNDC | CS_DBLCLKS;
		description.lpfnWndProc = &Window::_procedure;
		description.hInstance = _instance;
		description.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
		description.lpszClassName = _identifier.c_str();
		_atom = ::RegisterClassExW(&description);
		if (_atom == 0)
		{
			Window::_throwLastError("RegisterClassExW");
		}
	}

	[[noreturn]] void Window::_throwLastError(std::string_view operation)
	{
		throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), std::string(operation));
	}

	int Window::_wideSize(std::string_view value)
	{
		const int result = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
		if (result == 0)
		{
			_throwLastError("MultiByteToWideChar");
		}
		return result;
	}

	void Window::_writeWide(std::string_view value, std::wstring &result)
	{
		const int converted = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), static_cast<int>(result.size()));
		if (converted == 0)
		{
			_throwLastError("MultiByteToWideChar");
		}
	}

	std::wstring Window::_toWide(std::string_view value)
	{
		if (value.empty())
		{
			return {};
		}
		std::wstring result(static_cast<std::size_t>(_wideSize(value)), L'\0');
		_writeWide(value, result);
		return result;
	}

	SIZE Window::_outerSize(const CreationInfo &info)
	{
		RECT rectangle{.left = 0, .top = 0, .right = static_cast<LONG>(info.width), .bottom = static_cast<LONG>(info.height)};
		if (::AdjustWindowRectEx(&rectangle, Style, FALSE, 0) == FALSE)
		{
			_throwLastError("AdjustWindowRectEx");
		}
		return {.cx = rectangle.right - rectangle.left, .cy = rectangle.bottom - rectangle.top};
	}

	Window *Window::_instance(HWND handle) noexcept
	{
		return reinterpret_cast<Window *>(::GetWindowLongPtrW(handle, GWLP_USERDATA));
	}

	Window *Window::_bind(HWND handle, LPARAM parameter) noexcept
	{
		auto *creation = reinterpret_cast<CREATESTRUCTW *>(parameter);
		auto *window = static_cast<Window *>(creation->lpCreateParams);
		if (window == nullptr)
		{
			return nullptr;
		}
		::SetLastError(ERROR_SUCCESS);
		const LONG_PTR previous = ::SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		if (previous == 0 && ::GetLastError() != ERROR_SUCCESS)
		{
			return nullptr;
		}
		window->_handle = handle;
		return window;
	}

	LRESULT Window::_process(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		if (message == WM_CLOSE)
		{
			_closureRequested = true;
			return 0;
		}
		if (message == WM_NCDESTROY)
		{
			return _processDestruction(handle, message, wParam, lParam);
		}
		return _messageHandler ? _messageHandler(handle, message, wParam, lParam) : ::DefWindowProcW(handle, message, wParam, lParam);
	}

	LRESULT Window::_processDestruction(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		const LRESULT result = ::DefWindowProcW(handle, message, wParam, lParam);
		::SetWindowLongPtrW(handle, GWLP_USERDATA, 0);
		_handle = nullptr;
		return result;
	}

	LRESULT CALLBACK Window::_procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		Window *window = _instance(handle);
		if (message == WM_NCCREATE)
		{
			window = _bind(handle, lParam);
		}
		if (window == nullptr)
		{
			return message == WM_NCCREATE ? FALSE : ::DefWindowProcW(handle, message, wParam, lParam);
		}
		try
		{
			return window->_process(handle, message, wParam, lParam);
		} catch (...)
		{
			if (window->_pendingException == nullptr)
			{
				window->_pendingException = std::current_exception();
			}
			return 0;
		}
	}

	void Window::_createHandle(const Class &windowClass, const CreationInfo &info, SIZE size)
	{
		const std::wstring title = _toWide(info.title);
		const HWND handle = ::CreateWindowExW(0, windowClass._identifier.c_str(), title.c_str(), Style, info.x, info.y, size.cx, size.cy, nullptr, nullptr, windowClass._instance, this);
		if (handle == nullptr)
		{
			_throwLastError("CreateWindowExW");
		}
		_handle = handle;
	}

	void Window::create(const Class &windowClass, const CreationInfo &info)
	{
		if (_handle != nullptr)
		{
			throw std::logic_error("The WinAPI window is already created");
		}
		_closureRequested = false;
		_closureDispatched = false;
		_messageHandler = info.messageHandler;
		_createHandle(windowClass, info, _outerSize(info));
		if (info.visible)
		{
			::ShowWindow(_handle, SW_SHOW);
		}
	}

	void Window::destroy()
	{
		if (_handle == nullptr)
		{
			return;
		}
		if (::DestroyWindow(_handle) == FALSE)
		{
			_throwLastError("DestroyWindow");
		}
	}

	bool Window::consumeClosureRequest() noexcept
	{
		if (!_closureRequested || _closureDispatched)
		{
			return false;
		}
		_closureDispatched = true;
		return true;
	}

	HWND Window::handle() const noexcept
	{
		return _handle;
	}

	void Window::rethrowPendingException()
	{
		if (_pendingException == nullptr)
		{
			return;
		}
		std::exception_ptr exception = std::exchange(_pendingException, nullptr);
		std::rethrow_exception(exception);
	}

	spk::Rect2D Window::geometry() const
	{
		if (_handle == nullptr)
		{
			throw std::logic_error("Cannot retrieve the geometry of an uninitialized WinAPI frame");
		}

		RECT area{};
		if (::GetClientRect(_handle, &area) == FALSE)
		{
			_throwLastError("GetClientRect");
		}

		return spk::Rect2D{
			.anchor = {area.left, area.top},
			.size = {
				static_cast<std::uint32_t>(area.right - area.left),
				static_cast<std::uint32_t>(area.bottom - area.top)}};
	}
}
