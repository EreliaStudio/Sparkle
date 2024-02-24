#include "application/spk_application.hpp"

namespace spk
{
	void GLAPIENTRY
	OpenGLDebugMessageCallback(GLenum source,
							   GLenum type,
							   GLuint id,
							   GLenum severity,
							   GLsizei length,
							   const GLchar *message,
							   const void *userParam)
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
			throwException("Unexpected opengl error detected");
	}

	Application::Handle::Handle(Application *p_application, const std::string& p_title, const Vector2UInt& p_size) :
		_size(p_size)
	{
		_applicationInstance = GetModuleHandle(NULL);

		_frameClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		_frameClass.lpfnWndProc = (WNDPROC) WindowProc;
		_frameClass.cbClsExtra = 0;
		_frameClass.cbWndExtra = 0;
		_frameClass.hInstance = _applicationInstance;
		_frameClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
		_frameClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		_frameClass.hbrBackground = NULL;
		_frameClass.lpszMenuName = NULL;
		_frameClass.lpszClassName = p_title.c_str();

		RegisterClass(&_frameClass);

		RECT windowSize;
		windowSize.left = (long)0;
		windowSize.right = (long)p_size.x;
		windowSize.top = (long)0;
		windowSize.bottom = (long)p_size.y;
		
		DWORD _windowStyle;
		_windowStyle = WS_OVERLAPPEDWINDOW;

		DWORD _windowExStyle;
		_windowExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

		AdjustWindowRectEx(&windowSize, _windowStyle, FALSE, _windowExStyle);

		_windowHandle = CreateWindowEx(
			_windowExStyle,
			p_title.c_str(), p_title.c_str(),
			_windowStyle,
			CW_USEDEFAULT, CW_USEDEFAULT,
			p_size.x, p_size.y,
			NULL, NULL,
			_applicationInstance,
			p_application);

		ShowWindow(_windowHandle, SW_SHOW);
		UpdateWindow(_windowHandle);

		_deviceContext = GetDC(GetForegroundWindow());

		memset(&_pixelFormatDescriptor, 0, sizeof(PIXELFORMATDESCRIPTOR));

		_pixelFormatDescriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		_pixelFormatDescriptor.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
		_pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
		_pixelFormatDescriptor.cColorBits = 24;
		_pixelFormatDescriptor.cDepthBits = 32;
		_pixelFormatDescriptor.cStencilBits = 32;
		_pixelFormatDescriptor.iLayerType = PFD_MAIN_PLANE;

		int nPixelFormat = ChoosePixelFormat(_deviceContext, &_pixelFormatDescriptor);
		SetPixelFormat(_deviceContext, nPixelFormat, &_pixelFormatDescriptor);

		HGLRC tempOpenGLContext = wglCreateContext(_deviceContext);
		wglMakeCurrent(_deviceContext, tempOpenGLContext);

		glewInit();

		int attributes[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 0,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			0};

		if (wglewIsSupported("WGL_ARB_create_context") == GL_FALSE)
		{
			_openglContext = wglCreateContextAttribsARB(_deviceContext, NULL, attributes);
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(tempOpenGLContext);
			wglMakeCurrent(_deviceContext, _openglContext);
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
		glDepthFunc(GL_LESS);

		glDisable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilMask(0xFF);

		glDisable(GL_SCISSOR_TEST);

		wglSwapIntervalEXT(0);
	}

	Application::Handle::~Handle()
	{
		if (_windowHandle != NULL) {
			DestroyWindow(_windowHandle);
			_windowHandle = NULL;
		}
		UnregisterClass(_frameClass.lpszClassName, _applicationInstance);
	}

	void Application::Handle::clear()
	{
		glViewport(0, 0, _size.x, _size.y);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	void Application::Handle::swap()
	{
		SwapBuffers(_deviceContext);
	}

	void Application::Handle::resize(const Vector2UInt& p_newSize)
	{
		_size = p_newSize;
	}

	void Application::Handle::place(const Vector2Int& p_anchor)
	{
		_anchor = p_anchor;
	}

	const spk::Vector2UInt& Application::Handle::size() const
	{
		return (_size);
	}
}