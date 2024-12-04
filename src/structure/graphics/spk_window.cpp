#include "structure/graphics/spk_window.hpp"

#include "application/spk_graphical_application.hpp"

#include "spk_debug_macro.hpp"
#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#include "utils/spk_string_utils.hpp"

namespace spk
{
	void Window::_initialize(const std::function<void(spk::SafePointer<spk::Window>)>& p_onClosureCallback)
	{
		_windowRendererThread.start();
		_windowUpdaterThread.start();
		_controllerInputThread.start();
		_onClosureCallback = p_onClosureCallback;
	}

	LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		Window* window = nullptr;

		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<Window*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		

		if (window != nullptr)
		{
			if (uMsg == WM_TIMER && wParam == 1)
			{
				window->requestUpdate();
				return 0;
			}

			if (window->_receiveEvent(uMsg, wParam, lParam) == true)
				return (0);
		}

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	bool Window::_receiveEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (_subscribedModules.contains(uMsg) == false)
			return (false);
		_subscribedModules[uMsg]->receiveEvent(spk::Event(this, uMsg, wParam, lParam));
		return (true);
	}

	void Window::_createContext()
	{
		RECT adjustedRect = {
			static_cast<LONG>(0),
			static_cast<LONG>(0),
			static_cast<LONG>(_viewport.geometry().width),
			static_cast<LONG>(_viewport.geometry().heigth)
		};
		if (!AdjustWindowRectEx(&adjustedRect, WS_OVERLAPPEDWINDOW, FALSE, 0))
		{
			throw std::runtime_error("Failed to adjust window rect.");
		}
		
		std::string convertedTitle = spk::StringUtils::wstringToString(_title);

		_hwnd = CreateWindowEx(
			0,
			"SPKWindowClass",
			convertedTitle.c_str(),
			WS_OVERLAPPEDWINDOW,
			_viewport.geometry().x, _viewport.geometry().y,
			adjustedRect.right - adjustedRect.left,
			adjustedRect.bottom - adjustedRect.top,
			nullptr, nullptr, GetModuleHandle(nullptr), this);

		if (!_hwnd)
		{
			throw std::runtime_error("Failed to create window.");
		}

		if (_pendingUpdateRate.has_value())
		{
			setUpdateRate(_pendingUpdateRate.value());
			_pendingUpdateRate.reset();
		}
		
		_controllerInputThread.bind(_hwnd);
		_createOpenGLContext();
		
		ShowWindow(_hwnd, SW_SHOW);
		UpdateWindow(_hwnd);
	}

	void Window::setUpdateRate(const long long& p_durationInMillisecond)
	{
		if (_updateTimerID != 0)
		{
			clearUpdateRate();
		}

		if (_hwnd == nullptr)
		{
			_pendingUpdateRate = p_durationInMillisecond;
			return;
		}

		MSG msg;
		PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);

		_updateTimerID = SetTimer(_hwnd, 1, static_cast<UINT>(p_durationInMillisecond), nullptr);
		if (_updateTimerID == 0)
		{
			DWORD errorCode = GetLastError();
			throw std::runtime_error("Failed to set update timer. Error code: " + std::to_string(errorCode));
		}
	}

	void Window::clearUpdateRate()
	{
		if (_updateTimerID == 0 && !_pendingUpdateRate.has_value())
		{
			throw std::runtime_error("No active or pending timer to clear.");
		}

		if (_updateTimerID != 0)
		{
			if (!KillTimer(_hwnd, _updateTimerID))
			{
				throw std::runtime_error("Failed to clear update timer.");
			}
			_updateTimerID = 0;
		}

		_pendingUpdateRate.reset();
	}

	void GLAPIENTRY
		OpenGLDebugMessageCallback(GLenum source,
			GLenum type,
			GLuint id,
			GLenum severity,
			GLsizei length,
			const GLchar* message,
			const void* userParam)
	{
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
			return;

		std::cout << "---------------" << std::endl;

		switch (source)
		{
		case GL_DEBUG_SOURCE_API:
			std::cout << "Source: API" << std::endl;
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			std::cout << "Source: Window System" << std::endl;
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			std::cout << "Source: Shader Compiler" << std::endl;
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			std::cout << "Source: Third Party" << std::endl;
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			std::cout << "Source: Application" << std::endl;
			break;
		case GL_DEBUG_SOURCE_OTHER:
			std::cout << "Source: Other" << std::endl;
			break;
		}

		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
			std::cout << "Type: Error" << std::endl;
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			std::cout << "Type: Deprecated Behaviour" << std::endl;
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			std::cout << "Type: Undefined Behaviour" << std::endl;
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			std::cout << "Type: Portability" << std::endl;
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			std::cout << "Type: Performance" << std::endl;
			break;
		case GL_DEBUG_TYPE_MARKER:
			std::cout << "Type: Marker" << std::endl;
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP:
			std::cout << "Type: Push Group" << std::endl;
			break;
		case GL_DEBUG_TYPE_POP_GROUP:
			std::cout << "Type: Pop Group" << std::endl;
			break;
		case GL_DEBUG_TYPE_OTHER:
			std::cout << "Type: Other" << std::endl;
			break;
		}

		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:
			std::cout << "Severity: high" << std::endl;
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			std::cout << "Severity: medium" << std::endl;
			break;
		case GL_DEBUG_SEVERITY_LOW:
			std::cout << "Severity: low" << std::endl;
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			std::cout << "Severity: notification" << std::endl;
			break;
		}
			
		std::cout << "Debug message (" << id << "): " << message << std::endl;
		if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
			throw std::runtime_error("Unexpected opengl error detected");
	}

	void Window::_createOpenGLContext()
	{
		_hdc = GetDC(_hwnd);
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			32,
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,
			8,
			0,
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

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
		int attributes[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 5,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0 };

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
		glDebugMessageCallback(OpenGLDebugMessageCallback, 0);

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

	Window::Window(const std::wstring& p_title, const spk::Geometry2D& p_geometry) :
		_rootWidget(std::make_unique<Widget>(p_title + L" - CentralWidget")),
		_title(p_title),
		_viewport(p_geometry),
		_windowRendererThread(p_title + L" - Renderer"),
		_windowUpdaterThread(p_title + L" - Updater")
	{
		_rootWidget->setGeometry(p_geometry);
		_windowRendererThread.addPreparationStep([&]() {
				try
				{
					_createContext();
				}
				catch (std::exception& e)
				{
					std::cout << "Error catched : " << e.what() << std::endl;
					close();
				}
			}).relinquish();
		_windowRendererThread.addExecutionStep([&]() {
				try
				{
					pullEvents();
					paintModule.treatMessages();
					systemModule.treatMessages();
				}
				catch (std::exception& e)
				{
					std::cout << "Renderer - Error catched : " << e.what() << std::endl;
					close();
				}
			}).relinquish();
		_windowUpdaterThread.addExecutionStep([&]() {
				try
				{
					mouseModule.treatMessages();
					keyboardModule.treatMessages();
					controllerModule.treatMessages();
					updateModule.treatMessages();
				}
				catch (std::exception& e)
				{
					std::cout << "Updater - Error catched : " << e.what() << std::endl;
					close();
				}
			}).relinquish();

		bindModule(&mouseModule);
		bindModule(&keyboardModule);
		bindModule(&controllerModule);
		bindModule(&updateModule);
		bindModule(&paintModule);
		bindModule(&systemModule);

		mouseModule.linkToWidget(_rootWidget.get());
		keyboardModule.linkToWidget(_rootWidget.get());
		controllerModule.linkToWidget(_rootWidget.get());
		updateModule.linkToWidget(_rootWidget.get());
		paintModule.linkToWidget(_rootWidget.get());

		updateModule.linkToController(&(controllerModule.controller()));
		updateModule.linkToMouse(&(mouseModule.mouse()));
		updateModule.linkToKeyboard(&(keyboardModule.keyboard()));

		_rootWidget->activate();
	}

	Window::~Window()
	{
		_rootWidget.release();
	}

	void Window::move(const spk::Geometry2D::Point& p_newPosition)
	{
		_viewport.setGeometry({ 0, 0, _viewport.geometry().size });
		_rootWidget->setGeometry(_viewport.geometry());
	}
	
	void Window::resize(const spk::Geometry2D::Size& p_newSize)
	{
		if (p_newSize.x == 0 || p_newSize.y == 0)
			return ;

		_viewport.setGeometry({ 0, 0, p_newSize});
		_rootWidget->setGeometry(_viewport.geometry());
		for (auto& child : _rootWidget->children())
		{
			child->_resize();
		}
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
			_onClosureCallback(this);
	}

	void Window::clear()
	{
		_viewport.apply();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	void Window::swap() const
	{
		SwapBuffers(_hdc);
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
	
	void Window::bindModule(spk::IModule* p_module)
	{
		for (const auto& ID : p_module->eventIDs())
		{
			_subscribedModules[ID] = p_module;
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

	const std::wstring& Window::title() const
	{
		return (_title);
	}
	
	const spk::Geometry2D& Window::geometry() const
	{
		return (_viewport.geometry());
	}

	void Window::requestPaint() const
	{
		PostMessage(_hwnd, WM_PAINT_REQUEST, 0, 0);
	}

	void Window::requestUpdate() const
	{
		PostMessage(_hwnd, WM_UPDATE_REQUEST, 0, 0);
	}
}