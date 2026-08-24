#include "core/window.hpp"
#include "core/platform/window.hpp"


#include <GL/glew.h>
#include <Windows.h>

#include <atomic>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "graphics/opengl/gpu_resource_collection.hpp"

namespace spk
{
	struct Window::Surface::Impl
	{
		using CreateContextAttribs = HGLRC(WINAPI *)(HDC, HGLRC, const int *);
		static constexpr int OpenGLMajorVersion = 4;
		static constexpr int OpenGLMinorVersion = 6;
		static constexpr int ContextMajorVersionAttribute = 0x2091;
		static constexpr int ContextMinorVersionAttribute = 0x2092;
		static constexpr int ContextFlagsAttribute = 0x2094;
		static constexpr int ContextProfileMaskAttribute = 0x9126;
		static constexpr int ContextDebugBit = 0x0001;
		static constexpr int ContextCoreProfileBit = 0x00000001;

		Window::Identifier windowID;
		std::atomic<LifeCycle> lifeCycle = LifeCycle::Pending;
		std::unique_ptr<GPUResourceCollection> _gpuResources;
		HWND windowHandle = nullptr;
		HDC deviceContext = nullptr;
		HGLRC renderingContext = nullptr;
		spk::Rect2D geometry;

		explicit Impl(Window::Identifier windowID) :
			windowID(std::move(windowID)),
			_gpuResources(std::make_unique<GPUResourceCollection>())
		{
		}

		[[noreturn]] static void throwLastError(std::string_view operation)
		{
			throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), std::string(operation));
		}

		[[nodiscard]] static bool isValidProcedure(PROC procedure) noexcept
		{
			return procedure != nullptr && procedure != reinterpret_cast<PROC>(1) &&
				   procedure != reinterpret_cast<PROC>(2) && procedure != reinterpret_cast<PROC>(3) &&
				   procedure != reinterpret_cast<PROC>(-1);
		}

		[[nodiscard]] static PIXELFORMATDESCRIPTOR pixelFormatDescriptor() noexcept
		{
			PIXELFORMATDESCRIPTOR result{};
			result.nSize = sizeof(result);
			result.nVersion = 1;
			result.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
			result.iPixelType = PFD_TYPE_RGBA;
			result.cColorBits = 32;
			result.cDepthBits = 24;
			result.cStencilBits = 8;
			result.iLayerType = PFD_MAIN_PLANE;
			return result;
		}

		void acquireDeviceContext(const WinAPI::Window &frame)
		{
			windowHandle = frame.handle();
			if (windowHandle == nullptr)
			{
				throw std::logic_error("Cannot create an OpenGL surface without a native window");
			}
			deviceContext = ::GetDC(windowHandle);
			if (deviceContext == nullptr)
			{
				throwLastError("GetDC");
			}
		}

		void setPixelFormat()
		{
			const PIXELFORMATDESCRIPTOR descriptor = pixelFormatDescriptor();
			const int format = ::ChoosePixelFormat(deviceContext, &descriptor);
			if (format == 0)
			{
				throwLastError("ChoosePixelFormat");
			}
			if (::SetPixelFormat(deviceContext, format, &descriptor) == FALSE)
			{
				throwLastError("SetPixelFormat");
			}
		}

		[[nodiscard]] HGLRC createBootstrapContext()
		{
			HGLRC result = ::wglCreateContext(deviceContext);
			if (result == nullptr)
			{
				throwLastError("wglCreateContext");
			}
			if (::wglMakeCurrent(deviceContext, result) == FALSE)
			{
				::wglDeleteContext(result);
				throwLastError("wglMakeCurrent");
			}
			return result;
		}

		[[nodiscard]] static CreateContextAttribs loadContextFactory()
		{
			const PROC procedure = ::wglGetProcAddress("wglCreateContextAttribsARB");
			if (!isValidProcedure(procedure))
			{
				throw std::runtime_error("WGL_ARB_create_context is not supported");
			}
			return reinterpret_cast<CreateContextAttribs>(procedure);
		}

		[[nodiscard]] HGLRC createRenderingContext(CreateContextAttribs factory)
		{
#ifdef _DEBUG
			constexpr int flags = ContextDebugBit;
#else
			constexpr int flags = 0;
#endif
			const int attributes[] = {ContextMajorVersionAttribute, OpenGLMajorVersion, ContextMinorVersionAttribute, OpenGLMinorVersion, ContextProfileMaskAttribute, ContextCoreProfileBit, ContextFlagsAttribute, flags, 0};
			HGLRC result = factory(deviceContext, nullptr, attributes);
			if (result == nullptr)
			{
				throwLastError("wglCreateContextAttribsARB");
			}
			return result;
		}

		static void deleteContext(HGLRC context)
		{
			if (context != nullptr && ::wglDeleteContext(context) == FALSE)
			{
				throwLastError("wglDeleteContext");
			}
		}

		void destroyBootstrapContext(HGLRC context)
		{
			if (::wglMakeCurrent(nullptr, nullptr) == FALSE)
			{
				throwLastError("wglMakeCurrent");
			}
			deleteContext(context);
		}

		void releaseRenderingContext()
		{
			if (renderingContext == nullptr)
			{
				return;
			}

			if (::wglGetCurrentContext() != renderingContext)
			{
				if (::wglMakeCurrent(deviceContext, renderingContext) == FALSE)
				{
					throwLastError("wglMakeCurrent");
				}
			}

			_gpuResources->clear();

			if (::wglMakeCurrent(nullptr, nullptr) == FALSE)
			{
				throwLastError("wglMakeCurrent");
			}

			deleteContext(renderingContext);
			renderingContext = nullptr;
		}

		void releaseDeviceContext() noexcept
		{
			if (deviceContext != nullptr && windowHandle != nullptr)
			{
				::ReleaseDC(windowHandle, deviceContext);
			}
			deviceContext = nullptr;
		}

		void cleanupAfterCreationFailure(HGLRC bootstrap) noexcept
		{
			::wglMakeCurrent(nullptr, nullptr);
			if (renderingContext != nullptr)
			{
				::wglDeleteContext(renderingContext);
			}
			if (bootstrap != nullptr)
			{
				::wglDeleteContext(bootstrap);
			}
			renderingContext = nullptr;
			releaseDeviceContext();
			windowHandle = nullptr;
		}
	};

	Window::Surface::Surface(const Window::Identifier &windowID) :
		_impl(std::make_unique<Impl>(windowID))
	{
	}
	Window::Surface::~Surface() = default;

	Window::LifeCycle Window::Surface::lifeCycle() const noexcept
	{
		return _impl->lifeCycle.load();
	}

	static void initializeGLEW()
	{
		glewExperimental = GL_TRUE;

		const GLenum result = glewInit();
		if (result != GLEW_OK)
		{
			throw std::runtime_error(
				"Failed to initialize GLEW: " +
				std::string(reinterpret_cast<const char *>(glewGetErrorString(result))));
		}
	}

	void Window::Surface::create(const WinAPI::Window &frame)
	{
		if (_impl->lifeCycle != LifeCycle::Pending)
		{
			throw std::logic_error("The OpenGL surface cannot be created in its current state");
		}
		HGLRC bootstrap = nullptr;
		try
		{
			_impl->geometry = frame.geometry();
			_impl->acquireDeviceContext(frame);
			_impl->setPixelFormat();
			bootstrap = _impl->createBootstrapContext();
			_impl->renderingContext = _impl->createRenderingContext(Impl::loadContextFactory());
			_impl->destroyBootstrapContext(bootstrap);
			bootstrap = nullptr;
			makeCurrent();

			initializeGLEW();

			_impl->lifeCycle = LifeCycle::Ready;
		} catch (...)
		{
			_impl->cleanupAfterCreationFailure(bootstrap);
			throw;
		}
	}

	void Window::Surface::destroy()
	{
		if (_impl->lifeCycle == LifeCycle::Released)
		{
			return;
		}

		_impl->lifeCycle = LifeCycle::Releasing;

		_impl->releaseRenderingContext();

		_impl->releaseDeviceContext();

		_impl->windowHandle = nullptr;

		_impl->lifeCycle = LifeCycle::Released;
	}

	void Window::Surface::makeCurrent()
	{
		if (_impl->renderingContext == nullptr)
		{
			throw std::logic_error("Cannot activate an uninitialized OpenGL surface");
		}
		if (::wglGetCurrentContext() == _impl->renderingContext && ::wglGetCurrentDC() == _impl->deviceContext)
		{
			return;
		}
		if (::wglMakeCurrent(_impl->deviceContext, _impl->renderingContext) == FALSE)
		{
			Impl::throwLastError("wglMakeCurrent");
		}
	}

	void Window::Surface::setGeometry(const spk::Rect2D &geometry) noexcept
	{
		_impl->geometry = geometry;
	}

	const spk::Rect2D &Window::Surface::geometry() const noexcept
	{
		return _impl->geometry;
	}

	GPUResourceCollection &Window::Surface::_gpuResources()
	{
		return *_impl->_gpuResources;
	}

	void Window::Surface::present()
	{
		if (_impl->deviceContext == nullptr)
		{
			throw std::logic_error("Cannot present an uninitialized OpenGL surface");
		}
		if (::SwapBuffers(_impl->deviceContext) == FALSE)
		{
			Impl::throwLastError("SwapBuffers");
		}
	}
}
