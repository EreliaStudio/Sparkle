#include "structure/graphics/spk_window.hpp"

#include "application/spk_graphical_application.hpp"

#include <GL/glew.h>
#ifdef _WIN32
#include <GL/wglew.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#else
#include <GL/glx.h>
#endif

#include "utils/spk_string_utils.hpp"

#include "structure/spk_iostream.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	void Window::_initialize(const std::function<void(spk::SafePointer<spk::Window>)> &p_onClosureCallback)
	{
		_windowRendererThread.start();
		_windowUpdaterThread.start();
		_controllerInputThread.start();
		_onClosureCallback = p_onClosureCallback;
	}

#ifdef _WIN32
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
#endif

	bool Window::_receiveEvent(UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
	{
#ifdef _WIN32
               if (p_uMsg == WM_SETCURSOR)
               {
                       if (LOWORD(p_lParam) == HTCLIENT)
                       {
                               ::SetCursor(_currentCursor);
                               return true;
                       }

                       return false;
               }
#endif

		if (_subscribedModules.contains(p_uMsg) == false)
		{
			return (false);
		}
		_subscribedModules[p_uMsg]->receiveEvent(spk::Event(this, p_uMsg, p_wParam, p_lParam));
		return (true);
	}

       void Window::_createContext()
       {
#ifdef _WIN32
               RECT adjustedRect = {static_cast<LONG>(0),
                                                        static_cast<LONG>(0),
                                                        static_cast<LONG>(_rootWidget->viewport().geometry().width),
                                                        static_cast<LONG>(_rootWidget->viewport().geometry().height)};
               if (!AdjustWindowRectEx(&adjustedRect, WS_OVERLAPPEDWINDOW, FALSE, 0))
               {
                       GENERATE_ERROR("Failed to adjust window rect.");
               }

               std::string convertedTitle = spk::StringUtils::wstringToString(_title);

               _hwnd = CreateWindowEx(0,
                                                          "SPKWindowClass",
                                                          convertedTitle.c_str(),
                                                          WS_OVERLAPPEDWINDOW,
                                                          _rootWidget->viewport().geometry().x,
                                                          _rootWidget->viewport().geometry().y,
                                                          adjustedRect.right - adjustedRect.left,
                                                          adjustedRect.bottom - adjustedRect.top,
                                                          nullptr,
                                                          nullptr,
                                                          GetModuleHandle(nullptr),
                                                          this);

               if (!_hwnd)
               {
                       GENERATE_ERROR("Failed to create window.");
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
#else
               _display = XOpenDisplay(nullptr);
               if (!_display)
               {
                       GENERATE_ERROR("Failed to open X display");
               }
               int screen = DefaultScreen(_display);
               GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
               XVisualInfo* vi = glXChooseVisual(_display, screen, att);
               if (!vi)
               {
                       GENERATE_ERROR("No appropriate visual found");
               }
               Colormap cmap = XCreateColormap(_display, RootWindow(_display, screen), vi->visual, AllocNone);
               XSetWindowAttributes swa;
               swa.colormap = cmap;
               swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
                                 PointerMotionMask | StructureNotifyMask | FocusChangeMask;
               _window = XCreateWindow(_display,
                                      RootWindow(_display, screen),
                                      _rootWidget->viewport().geometry().x,
                                      _rootWidget->viewport().geometry().y,
                                      _rootWidget->viewport().geometry().width,
                                      _rootWidget->viewport().geometry().height,
                                      0,
                                      vi->depth,
                                      InputOutput,
                                      vi->visual,
                                      CWColormap | CWEventMask,
                                      &swa);

               Atom wmDelete = XInternAtom(_display, "WM_DELETE_WINDOW", False);
               XSetWMProtocols(_display, _window, &wmDelete, 1);

               std::string convertedTitle = spk::StringUtils::wstringToString(_title);
               XStoreName(_display, _window, convertedTitle.c_str());

               _glxContext = glXCreateContext(_display, vi, NULL, GL_TRUE);
               glXMakeCurrent(_display, _window, _glxContext);
               glewInit();

               _cursors[L"Arrow"] = XCreateFontCursor(_display, XC_left_ptr);
               setCursor(L"Arrow");

               XMapWindow(_display, _window);
#endif
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
			GENERATE_ERROR("Failed to set update timer. Error code: " + std::to_string(errorCode));
		}
		_timers.erase(result);
		return (result);
	}

	void Window::_deleteTimer(UINT_PTR p_id)
	{
		if (!KillTimer(_hwnd, p_id))
		{
			GENERATE_ERROR("Failed to clear update timer.");
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

       void Window::_createOpenGLContext()
       {
#ifdef _WIN32
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
			GENERATE_ERROR("Failed to choose pixel format.");
		}

		if (SetPixelFormat(_hdc, pixelFormat, &pfd) == FALSE)
		{
			GENERATE_ERROR("Failed to set pixel format.");
		}

		// Create a temporary context to initialize modern OpenGL
		HGLRC tempContext = wglCreateContext(_hdc);
		if (!tempContext)
		{
			GENERATE_ERROR("Failed to create temporary OpenGL context.");
		}

		if (!wglMakeCurrent(_hdc, tempContext))
		{
			GENERATE_ERROR("Failed to activate temporary OpenGL context.");
		}

		// Load OpenGL extensions
		glewExperimental = GL_TRUE;
		GLenum err = glewInit();
		if (err != GLEW_OK)
		{
			GENERATE_ERROR("Failed to initialize GLEW.");
		}

		// Load wglCreateContextAttribsARB extension
		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
			(PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
		if (!wglCreateContextAttribsARB)
		{
			GENERATE_ERROR("Failed to load wglCreateContextAttribsARB.");
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
			GENERATE_ERROR("Failed to create modern OpenGL context.");
		}

		// Delete the temporary context and activate the new one
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(tempContext);

		if (!wglMakeCurrent(_hdc, _hglrc))
		{
			GENERATE_ERROR("Failed to activate modern OpenGL context.");
		}

		// Optionally set VSync
		if (wglSwapIntervalEXT)
		{
			wglSwapIntervalEXT(1); // Enable VSync
		}

		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(spk::OpenGLUtils::openGLDebugMessageCallback, 0);

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

               glEnable(GL_SCISSOR_TEST);
               wglSwapIntervalEXT(0);
#else
               glewExperimental = GL_TRUE;
               glewInit();

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
               glEnable(GL_SCISSOR_TEST);
#endif
       }

       void Window::_destroyOpenGLContext()
       {
#ifdef _WIN32
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
#else
               if (_glxContext)
               {
                       glXMakeCurrent(_display, None, nullptr);
                       glXDestroyContext(_display, _glxContext);
                       _glxContext = nullptr;
               }

               if (_display)
               {
                       XDestroyWindow(_display, _window);
                       XCloseDisplay(_display);
                       _display = nullptr;
               }
#endif
       }

	Window::Window(const std::wstring &p_title, const spk::Geometry2D &p_geometry) :
		_rootWidget(std::make_unique<Widget>(p_title + L" - CentralWidget", nullptr)),
		_title(p_title),
		_windowRendererThread(p_title + L" - Renderer"),
		_windowUpdaterThread(p_title + L" - Updater"),
		_updateModule(_rootWidget.get())
	{
		_windowRendererThread
			.addPreparationStep(
				[&]()
				{
					try
					{
						_createContext();
						requestPaint();
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
						try
						{
							_handlePendingTimer();
						}
						catch (const std::exception& e)
						{
							PROPAGATE_ERROR("Renderer::_handlePendingTimer failed", e);
						}

						try
						{
							pullEvents();
						}
						catch (const std::exception& e)
						{
							PROPAGATE_ERROR("Renderer::pullEvents failed", e);
						}

						try
						{
							_timerModule.treatMessages();
						}
						catch (const std::exception& e)
						{
							PROPAGATE_ERROR("Renderer::_timerModule.treatMessages failed", e);
						}

						try
						{
							_paintModule.treatMessages();
						}
						catch (const std::exception& e)
						{
							PROPAGATE_ERROR("Renderer::_paintModule.treatMessages failed", e);
						}

						try
						{
							_systemModule.treatMessages();
						}
						catch (const std::exception& e)
						{
							PROPAGATE_ERROR("Renderer::_systemModule.treatMessages failed", e);
						}
					}
					catch (const std::exception& e)
					{
						spk::cout << "Renderer - Error caught:\n" << e.what() << std::endl;
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
						_mouseModule.treatMessages();
						_keyboardModule.treatMessages();
						_controllerModule.treatMessages();
						_updateModule.treatMessages();
					} catch (std::exception &e)
					{
						spk::cout << "Updater - Error catched : " << e.what() << std::endl;
						close();
					}
				})
			.relinquish();

		_updateModule.bind(&(_keyboardModule.keyboard()));
		_updateModule.bind(&(_mouseModule.mouse()));
		_updateModule.bind(&(_controllerModule.controller()));

		bindModule(&_mouseModule);
		bindModule(&_keyboardModule);
		bindModule(&_controllerModule);
		bindModule(&_updateModule);
		bindModule(&_paintModule);
		bindModule(&_systemModule);
		bindModule(&_timerModule);

		_rootWidget->setGeometry(p_geometry);
		_rootWidget->activate();
	}

	Window::~Window()
	{
		close();
		_removeAllTimers();
		_rootWidget.release();
	}

	void Window::move(const spk::Geometry2D::Point &p_newPosition)
	{
		// _rootWidget->viewport().setGeometry({ 0, 0, _rootWidget->viewport().geometry().size });
		// _rootWidget->setGeometry(_rootWidget->viewport().geometry());
	}

	void Window::resize(const spk::Geometry2D::Size &p_newSize)
	{
		if (p_newSize.x == 0 || p_newSize.y == 0)
        {
            return;
        }

		_rootWidget->forceGeometryChange({0, p_newSize});
		for (auto& child : _rootWidget->children())
		{
			_rootWidget->viewport().apply();
			child->_applyResize();
		}
	}

       void Window::close()
       {
               _windowRendererThread.stop();
               _windowUpdaterThread.stop();
               _controllerInputThread.stop();
#ifdef _WIN32
               if (_hwnd)
               {
                       DestroyWindow(_hwnd);
                       _hwnd = nullptr;
               }
#else
               if (_display)
               {
                       XDestroyWindow(_display, _window);
               }
#endif
               _destroyOpenGLContext();

		if (_onClosureCallback != nullptr)
		{
			_onClosureCallback(this);
		}
	}

	void Window::clear()
	{
		if (_rootWidget->_needGeometryChange == true)
		{
			try
			{
				_rootWidget->_applyGeometryChange();
			}
			catch (std::exception& e)
			{
				PROPAGATE_ERROR("Window::clear over _rootWidget->applyGeometryChange() failed", e);
			}
			
			_rootWidget->_needGeometryChange = false;
		}
		
		try
		{
			_rootWidget->viewport().apply();
		}
		catch (std::exception& e)
		{
			PROPAGATE_ERROR("Window::clear over _rootWidget->viewport().apply failed", e);
		}
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

       void Window::swap() const
       {
#ifdef _WIN32
               SwapBuffers(_hdc);
#else
               glXSwapBuffers(_display, _window);
#endif
       }

       void Window::addCursor(const std::wstring &p_cursorName, const std::filesystem::path &p_cursorPath)
       {
#ifdef _WIN32
               _cursors[p_cursorName] = LoadCursorFromFileW(p_cursorPath.c_str());
#else
               _cursors[p_cursorName] = XCreateFontCursor(_display, XC_left_ptr);
#endif
       }

       void Window::setCursor(const std::wstring &p_cursorName)
       {
               if (_cursors.contains(p_cursorName) == false)
               {
                       GENERATE_ERROR("Cursor [" + spk::StringUtils::wstringToString(p_cursorName) + "] not found.");
               }
#ifdef _WIN32
               HCURSOR nextCursor = _cursors[p_cursorName];
               if (_currentCursor == nextCursor)
               {
                       return;
               }
               _currentCursor = nextCursor;
#else
               Cursor nextCursor = _cursors[p_cursorName];
               if (_currentCursor == nextCursor)
               {
                       return;
               }
               _currentCursor = nextCursor;
               XDefineCursor(_display, _window, _currentCursor);
#endif
       }

       void Window::pullEvents()
       {
#ifdef _WIN32
               MSG msg;
               while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
               {
                       TranslateMessage(&msg);
                       DispatchMessage(&msg);
               }
#else
               XEvent evt;
               while (XPending(_display))
               {
                       XNextEvent(_display, &evt);
                       switch (evt.type)
                       {
                       case Expose:
                               _postedEvents.emplace_back(WM_PAINT_REQUEST, 0, 0);
                               break;
                       case ConfigureNotify:
                               _postedEvents.emplace_back(WM_SIZE, 0, MAKELPARAM(evt.xconfigure.width, evt.xconfigure.height));
                               break;
                       case KeyPress:
                               _postedEvents.emplace_back(WM_KEYDOWN, evt.xkey.keycode, 0);
                               break;
                       case KeyRelease:
                               _postedEvents.emplace_back(WM_KEYUP, evt.xkey.keycode, 0);
                               break;
                       case ButtonPress:
                               if (evt.xbutton.button == Button1)
                                       _postedEvents.emplace_back(WM_LBUTTONDOWN, 0, 0);
                               else if (evt.xbutton.button == Button3)
                                       _postedEvents.emplace_back(WM_RBUTTONDOWN, 0, 0);
                               else if (evt.xbutton.button == Button2)
                                       _postedEvents.emplace_back(WM_MBUTTONDOWN, 0, 0);
                               else if (evt.xbutton.button == 4)
                                       _postedEvents.emplace_back(WM_MOUSEWHEEL, MAKEWPARAM(0, 120), 0);
                               else if (evt.xbutton.button == 5)
                                       _postedEvents.emplace_back(WM_MOUSEWHEEL, MAKEWPARAM(0, -120), 0);
                               break;
                       case ButtonRelease:
                               if (evt.xbutton.button == Button1)
                                       _postedEvents.emplace_back(WM_LBUTTONUP, 0, 0);
                               else if (evt.xbutton.button == Button3)
                                       _postedEvents.emplace_back(WM_RBUTTONUP, 0, 0);
                               else if (evt.xbutton.button == Button2)
                                       _postedEvents.emplace_back(WM_MBUTTONUP, 0, 0);
                               break;
                       case MotionNotify:
                               _postedEvents.emplace_back(WM_MOUSEMOVE, 0, MAKELPARAM(evt.xmotion.x, evt.xmotion.y));
                               break;
                       case FocusIn:
                               _postedEvents.emplace_back(WM_SETFOCUS, 0, 0);
                               break;
                       case FocusOut:
                               _postedEvents.emplace_back(WM_KILLFOCUS, 0, 0);
                               break;
                       case ClientMessage:
                               _postedEvents.emplace_back(WM_CLOSE, 0, 0);
                               break;
                       }
               }
               while (_postedEvents.empty() == false)
               {
                       auto e = _postedEvents.front();
                       _postedEvents.pop_front();
                       _receiveEvent(std::get<0>(e), std::get<1>(e), std::get<2>(e));
               }
#endif
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
		return (_rootWidget->viewport().geometry());
	}

	void Window::allowPaintRequest()
	{
		_isPaintRequestAllowed = true;
	}

       void Window::requestPaint() const
       {
               if (_isPaintRequestAllowed == true)
               {
#ifdef _WIN32
                       PostMessage(_hwnd, WM_PAINT_REQUEST, 0, 0);
#else
                       _postedEvents.emplace_back(WM_PAINT_REQUEST, 0, 0);
#endif
                       _isPaintRequestAllowed = false;
               }
       }

       void Window::requestResize(const spk::Vector2Int& p_size) const
       {
#ifdef _WIN32
               PostMessage(_hwnd, WM_RESIZE_REQUEST, static_cast<WPARAM>(p_size.x), static_cast<LPARAM>(p_size.y));
#else
               _postedEvents.emplace_back(WM_RESIZE_REQUEST, static_cast<WPARAM>(p_size.x), static_cast<LPARAM>(p_size.y));
#endif
       }

       void Window::requestUpdate() const
       {
#ifdef _WIN32
               PostMessage(_hwnd, WM_UPDATE_REQUEST, 0, 0);
#else
               _postedEvents.emplace_back(WM_UPDATE_REQUEST, 0, 0);
#endif
       }
}