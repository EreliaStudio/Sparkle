#include "structure/graphics/spk_window.hpp"

#include "application/spk_graphical_application.hpp"

#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#include "utils/spk_string_utils.hpp"

#include "structure/spk_iostream.hpp"

namespace spk
{
	void Window::_initialize(const std::function<void(spk::SafePointer<spk::Window>)> &p_onClosureCallback)
	{
		_windowRendererThread.start();
		_windowUpdaterThread.start();
		_controllerInputThread.start();
		_onClosureCallback = p_onClosureCallback;
	}

	LRESULT CALLBACK Window::WindowProc(HWND p_hwnd, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
	{
		Window *window = nullptr;

		if (p_uMsg == WM_NCCREATE)
		{
			CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(p_lParam);
			window = reinterpret_cast<Window *>(pCreate->lpCreateParams);
			SetWindowLongPtr(p_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<Window *>(GetWindowLongPtr(p_hwnd, GWLP_USERDATA));
		}

		if (window != nullptr)
		{
			if (window->_receiveEvent(p_uMsg, p_wParam, p_lParam) == true)
			{
				return (TRUE);
			}
		}

		return DefWindowProc(p_hwnd, p_uMsg, p_wParam, p_lParam);
	}

	bool Window::_receiveEvent(UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
	{
		if (_subscribedModules.contains(p_uMsg) == false)
		{
			return (false);
		}
		_subscribedModules[p_uMsg]->receiveEvent(spk::Event(this, p_uMsg, p_wParam, p_lParam));
		return (true);
	}

	void Window::_createContext()
	{
		RECT adjustedRect = {static_cast<LONG>(0),
							 static_cast<LONG>(0),
							 static_cast<LONG>(_viewport.geometry().width),
							 static_cast<LONG>(_viewport.geometry().height)};
		if (!AdjustWindowRectEx(&adjustedRect, WS_OVERLAPPEDWINDOW, FALSE, 0))
		{
			throw std::runtime_error("Failed to adjust window rect.");
		}

		std::string convertedTitle = spk::StringUtils::wstringToString(_title);

		_hwnd = CreateWindowEx(0,
							   "SPKWindowClass",
							   convertedTitle.c_str(),
							   WS_OVERLAPPEDWINDOW,
							   _viewport.geometry().x,
							   _viewport.geometry().y,
							   adjustedRect.right - adjustedRect.left,
							   adjustedRect.bottom - adjustedRect.top,
							   nullptr,
							   nullptr,
							   GetModuleHandle(nullptr),
							   this);

		if (!_hwnd)
		{
			throw std::runtime_error("Failed to create window.");
		}

		_cursors[L"Arrow"] = ::LoadCursor(nullptr, IDC_ARROW);
		_cursors[L"TextEdit"] = ::LoadCursor(nullptr, IDC_IBEAM);
		_cursors[L"Wait"] = ::LoadCursor(nullptr, IDC_WAIT);
		_cursors[L"Cross"] = ::LoadCursor(nullptr, IDC_CROSS);
		_cursors[L"AltCross"] = ::LoadCursor(nullptr, IDC_UPARROW);
		_cursors[L"ResizeNWSE"] = ::LoadCursor(nullptr, IDC_SIZENWSE);
		_cursors[L"ResizeNESW"] = ::LoadCursor(nullptr, IDC_SIZENESW);
		_cursors[L"ResizeWE"] = ::LoadCursor(nullptr, IDC_SIZEWE);
		_cursors[L"ResizeNS"] = ::LoadCursor(nullptr, IDC_SIZENS);
		_cursors[L"Move"] = ::LoadCursor(nullptr, IDC_SIZEALL);
		_cursors[L"No"] = ::LoadCursor(nullptr, IDC_NO);
		_cursors[L"Hand"] = ::LoadCursor(nullptr, IDC_HAND);
		_cursors[L"Working"] = ::LoadCursor(nullptr, IDC_APPSTARTING);
		_cursors[L"Help"] = ::LoadCursor(nullptr, IDC_HELP);

		setUpdateTimer(1);

		setCursor(L"Arrow");

		_controllerInputThread.bind(_hwnd);
		_createOpenGLContext();

		ShowWindow(_hwnd, SW_SHOW);
		UpdateWindow(_hwnd);
	}

	void Window::_handlePendingTimer()
	{
		{
			std::lock_guard<std::recursive_mutex> lock(_timerMutex);

			if (_pendingTimerCreations.size() != 0)
			{
				for (const auto &pair : _pendingTimerCreations)
				{
					if (_timers.contains(pair.first))
					{
						_deleteTimer(pair.first);
					}
					_createTimer(pair.first, pair.second);
				}
				_pendingTimerCreations.clear();
			}

			if (_pendingTimerDeletions.size() != 0)
			{
				for (const auto &id : _pendingTimerDeletions)
				{
					_deleteTimer(id);
				}
				_pendingTimerDeletions.clear();
			}
		}
	}

	UINT_PTR Window::_createTimer(int p_id, const long long &p_durationInMillisecond)
	{
		UINT_PTR result = SetTimer(_hwnd, p_id, static_cast<UINT>(p_durationInMillisecond), nullptr);
		if (result == 0)
		{
			DWORD errorCode = GetLastError();
			throw std::runtime_error("Failed to set update timer. Error code: " + std::to_string(errorCode));
		}
		_timers.erase(result);
		return (result);
	}

	void Window::_deleteTimer(UINT_PTR p_id)
	{
		if (!KillTimer(_hwnd, p_id))
		{
			throw std::runtime_error("Failed to clear update timer.");
		}
		_timers.erase(p_id);
	}

	void Window::_removeAllTimers()
	{
		std::set<UINT_PTR> oldTimer = _timers;
		for (const auto &timer : oldTimer)
		{
			_deleteTimer(timer);
		}
	}

	void Window::setUpdateTimer(const long long &p_durationInMillisecond)
	{
		addTimer(-1, p_durationInMillisecond);
	}

	void Window::removeUpdateTimer()
	{
		removeTimer(-1);
	}

	void Window::addTimer(int p_id, const long long &p_durationInMillisecond)
	{
		std::lock_guard<std::recursive_mutex> lock(_timerMutex);
		std::pair<int, long long> value;

		value.first = p_id + 2;
		value.second = p_durationInMillisecond;

		_pendingTimerCreations.push_back(value);
	}

	void Window::removeTimer(int p_id)
	{
		_pendingTimerDeletions.push_back(p_id + 2);
	}

	void GLAPIENTRY openGLDebugMessageCallback(GLenum p_source, GLenum p_type, GLuint p_id, GLenum p_severity, GLsizei p_length, const GLchar *p_message,
											   const void *p_userParam)
	{
		if (p_id == 131169 || p_id == 131185 || p_id == 131218 || p_id == 131204)
		{
			return;
		}

		spk::cout << "---------------" << std::endl;

		switch (p_source)
		{
		case GL_DEBUG_SOURCE_API:
			spk::cout << "Source: API" << std::endl;
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			spk::cout << "Source: Window System" << std::endl;
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			spk::cout << "Source: Shader Compiler" << std::endl;
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			spk::cout << "Source: Third Party" << std::endl;
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			spk::cout << "Source: Application" << std::endl;
			break;
		case GL_DEBUG_SOURCE_OTHER:
			spk::cout << "Source: Other" << std::endl;
			break;
		}

		switch (p_type)
		{
		case GL_DEBUG_TYPE_ERROR:
			spk::cout << "Type: Error" << std::endl;
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			spk::cout << "Type: Deprecated Behaviour" << std::endl;
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			spk::cout << "Type: Undefined Behaviour" << std::endl;
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			spk::cout << "Type: Portability" << std::endl;
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			spk::cout << "Type: Performance" << std::endl;
			break;
		case GL_DEBUG_TYPE_MARKER:
			spk::cout << "Type: Marker" << std::endl;
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP:
			spk::cout << "Type: Push Group" << std::endl;
			break;
		case GL_DEBUG_TYPE_POP_GROUP:
			spk::cout << "Type: Pop Group" << std::endl;
			break;
		case GL_DEBUG_TYPE_OTHER:
			spk::cout << "Type: Other" << std::endl;
			break;
		}

		switch (p_severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:
			spk::cout << "Severity: high" << std::endl;
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			spk::cout << "Severity: medium" << std::endl;
			break;
		case GL_DEBUG_SEVERITY_LOW:
			spk::cout << "Severity: low" << std::endl;
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			spk::cout << "Severity: notification" << std::endl;
			break;
		}

		spk::cout << "Debug message (" << p_id << "): " << p_message << std::endl;
		if (p_severity != GL_DEBUG_SEVERITY_NOTIFICATION)
		{
			throw std::runtime_error("Unexpected opengl error detected");
		}
	}

	void Window::_createOpenGLContext()
	{
		_hdc = GetDC(_hwnd);
		PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR),
									 1,
									 PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
									 PFD_TYPE_RGBA,
									 32,
									 0,
									 0,
									 0,
									 0,
									 0,
									 0,
									 0,
									 0,
									 0,
									 0,
									 0,
									 0,
									 0,
									 24,
									 8,
									 0,
									 PFD_MAIN_PLANE,
									 0,
									 0,
									 0,
									 0};

		int pixelFormat = ChoosePixelFormat(_hdc, &pfd);
		if (pixelFormat == 0)
		{
			throw std::runtime_error("Failed to choose pixel format.");
		}

		if (SetPixelFormat(_hdc, pixelFormat, &pfd) == FALSE)
		{
			throw std::runtime_error("Failed to set pixel format.");
		}

		// Create a temporary context to initialize modern OpenGL
		HGLRC tempContext = wglCreateContext(_hdc);
		if (!tempContext)
		{
			throw std::runtime_error("Failed to create temporary OpenGL context.");
		}

		if (!wglMakeCurrent(_hdc, tempContext))
		{
			throw std::runtime_error("Failed to activate temporary OpenGL context.");
		}

		// Load OpenGL extensions
		glewExperimental = GL_TRUE;
		GLenum err = glewInit();
		if (err != GLEW_OK)
		{
			throw std::runtime_error("Failed to initialize GLEW.");
		}

		// Load wglCreateContextAttribsARB extension
		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
			(PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
		if (!wglCreateContextAttribsARB)
		{
			throw std::runtime_error("Failed to load wglCreateContextAttribsARB.");
		}

		// Now create a modern OpenGL context
		int attributes[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
							4,
							WGL_CONTEXT_MINOR_VERSION_ARB,
							5,
							WGL_CONTEXT_FLAGS_ARB,
							WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
							WGL_CONTEXT_PROFILE_MASK_ARB,
							WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
							0};

		_hglrc = wglCreateContextAttribsARB(_hdc, nullptr, attributes);
		if (!_hglrc)
		{
			throw std::runtime_error("Failed to create modern OpenGL context.");
		}

		// Delete the temporary context and activate the new one
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(tempContext);

		if (!wglMakeCurrent(_hdc, _hglrc))
		{
			throw std::runtime_error("Failed to activate modern OpenGL context.");
		}

		// Optionally set VSync
		if (wglSwapIntervalEXT)
		{
			wglSwapIntervalEXT(1); // Enable VSync
		}

		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(openGLDebugMessageCallback, 0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glClearDepth(1.0f);
		glDepthFunc(GL_LEQUAL);

		glDisable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilMask(0xFF);

		glDisable(GL_SCISSOR_TEST);

		wglSwapIntervalEXT(0);
	}

	void Window::_destroyOpenGLContext()
	{
		if (_hglrc)
		{
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(_hglrc);
			_hglrc = nullptr;
		}

		if (_hwnd && _hdc)
		{
			ReleaseDC(_hwnd, _hdc);
			_hdc = nullptr;
		}
	}

	Window::Window(const std::wstring &p_title, const spk::Geometry2D &p_geometry) :
		_rootWidget(std::make_unique<Widget>(p_title + L" - CentralWidget")),
		_title(p_title),
		_viewport(p_geometry),
		_windowRendererThread(p_title + L" - Renderer"),
		_windowUpdaterThread(p_title + L" - Updater")
	{
		resize(p_geometry.size);
		_windowRendererThread
			.addPreparationStep(
				[&]()
				{
					try
					{
						_createContext();
						requestResize();
					} catch (std::exception &e)
					{
						spk::cout << "Error catched : " << e.what() << std::endl;
						close();
					}
				})
			.relinquish();
		_windowRendererThread
			.addExecutionStep(
				[&]()
				{
					try
					{
						_handlePendingTimer();
						pullEvents();
						timerModule.treatMessages();
						paintModule.treatMessages();
						systemModule.treatMessages();
					} catch (std::exception &e)
					{
						spk::cout << "Renderer - Error catched : " << e.what() << std::endl;
						close();
					}
				})
			.relinquish();
		_windowUpdaterThread
			.addExecutionStep(
				[&]()
				{
					try
					{
						mouseModule.treatMessages();
						keyboardModule.treatMessages();
						controllerModule.treatMessages();
						updateModule.treatMessages();
					} catch (std::exception &e)
					{
						spk::cout << "Updater - Error catched : " << e.what() << std::endl;
						close();
					}
				})
			.relinquish();

		bindModule(&mouseModule);
		bindModule(&keyboardModule);
		bindModule(&controllerModule);
		bindModule(&updateModule);
		bindModule(&paintModule);
		bindModule(&systemModule);
		bindModule(&timerModule);

		_rootWidget->activate();
	}

	Window::~Window()
	{
		_removeAllTimers();
		_rootWidget.release();
	}

	void Window::move(const spk::Geometry2D::Point &p_newPosition)
	{
		// _viewport.setGeometry({ 0, 0, _viewport.geometry().size });
		// _rootWidget->setGeometry(_viewport.geometry());
	}

	void Window::resize(const spk::Geometry2D::Size &p_newSize)
	{
		if (p_newSize.x == 0 || p_newSize.y == 0)
		{
			return;
		}

		_viewport.setWindowSize(p_newSize);
		_rootWidget->_viewport.setWindowSize(p_newSize);

		_viewport.setGeometry({0, 0, p_newSize});
		_rootWidget->setGeometry(_viewport.geometry());

		for (auto &child : _rootWidget->children())
		{
			child->_resize();
		}

		requestPaint();
	}

	void Window::close()
	{
		_windowRendererThread.stop();
		_windowUpdaterThread.stop();
		_controllerInputThread.stop();

		if (_hwnd)
		{
			DestroyWindow(_hwnd);
			_hwnd = nullptr;
		}
		_destroyOpenGLContext();

		if (_onClosureCallback != nullptr)
		{
			_onClosureCallback(this);
		}
	}

	void Window::clear()
	{
		try
		{
			_viewport.apply();
		} catch (...)
		{
			throw std::runtime_error("Error while applying main window viewport");
		}
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	void Window::swap() const
	{
		SwapBuffers(_hdc);
	}

	void Window::addCursor(const std::wstring &p_cursorName, const std::filesystem::path &p_cursorPath)
	{
		_cursors[p_cursorName] = LoadCursorFromFileW(p_cursorPath.c_str());
	}

	void Window::setCursor(const std::wstring &p_cursorName)
	{
		if (_cursors.contains(p_cursorName) == false)
		{
			throw std::runtime_error("Cursor [" + spk::StringUtils::wstringToString(p_cursorName) + "] not found.");
		}

		HCURSOR nextCursor = _cursors[p_cursorName];
		if (_currentCursor == nextCursor)
		{
			return;
		}

		_currentCursor = nextCursor;
		::SetCursor(_currentCursor);
		::SendMessage(_hwnd, WM_SETCURSOR, reinterpret_cast<WPARAM>(_hwnd), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
	}

	void Window::_applyCursor()
	{
		::SetCursor(_currentCursor);
		_savedCursor = _currentCursor;
	}

	void Window::pullEvents()
	{
		MSG msg;

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void Window::bindModule(spk::IModule *p_module)
	{
		for (const auto &id : p_module->eventIDs())
		{
			_subscribedModules[id] = p_module;
		}
	}

	spk::SafePointer<Widget> Window::widget() const
	{
		return (_rootWidget.get());
	}

	Window::operator spk::SafePointer<Widget>() const
	{
		return (_rootWidget.get());
	}

	const std::wstring &Window::title() const
	{
		return (_title);
	}

	const spk::Geometry2D &Window::geometry() const
	{
		return (_viewport.geometry());
	}

	void Window::requestPaint() const
	{
		PostMessage(_hwnd, WM_PAINT_REQUEST, 0, 0);
	}

	void Window::requestResize() const
	{
		PostMessage(_hwnd, WM_RESIZE_REQUEST, 0, 0);
	}

	void Window::requestUpdate() const
	{
		PostMessage(_hwnd, WM_UPDATE_REQUEST, 0, 0);
	}
}