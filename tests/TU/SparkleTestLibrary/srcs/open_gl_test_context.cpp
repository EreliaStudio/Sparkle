#include "sparkle_test/open_gl_test_context.hpp"

#include <GL/glew.h>
#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <stb_image_write.h>

#include "core/platform/window.hpp"
#include "graphics/opengl/framebuffer.hpp"

namespace
{
	constexpr std::uint32_t FramebufferWidth = 640;
	constexpr std::uint32_t FramebufferHeight = 480;

	[[nodiscard]] std::string uniqueWindowClassName()
	{
		static std::atomic_uint64_t counter = 0;
		return "Sparkle_OpenGLTestContext_" + std::to_string(::GetCurrentProcessId()) + "_" +
			std::to_string(::GetTickCount64()) + "_" + std::to_string(counter.fetch_add(1));
	}
}

namespace sparkle_test
{
	struct OpenGLTestContext::Impl
	{
		spk::WinAPI::Window::Class windowClass{uniqueWindowClassName()};
		spk::Window::Native native{"OpenGL test native window"};
		spk::Window::Surface surface{"OpenGL test surface"};
		spk::RenderContext renderContext{.targetSurface = &surface};
		spk::Framebuffer framebuffer{{FramebufferWidth, FramebufferHeight}};
		std::thread::id ownerThread = std::this_thread::get_id();

		Impl()
		{
			spk::WinAPI::Window::CreationInfo creation;
			creation.title = "Sparkle OpenGL tests";
			creation.width = FramebufferWidth;
			creation.height = FramebufferHeight;
			creation.visible = false;

			native.window().create(windowClass, creation);
			try
			{
				surface.create(native.window());
				createFramebuffer();
			}
			catch (...)
			{
				try
				{
					surface.destroy();
				}
				catch (...)
				{
				}
				try
				{
					native.window().destroy();
				}
				catch (...)
				{
				}
				throw;
			}
		}

		~Impl() noexcept
		{
			try
			{
				surface.makeCurrent();
				surface.destroy();
			}
			catch (...)
			{
			}
			try
			{
				native.window().destroy();
			}
			catch (...)
			{
			}
		}

		void createFramebuffer()
		{
			framebuffer.activate(renderContext);
		}

		void requireOwnerThread() const
		{
			if (std::this_thread::get_id() != ownerThread)
			{
				throw std::logic_error("The shared OpenGL test context must be used from its creating thread");
			}
		}
	};

	const std::uint8_t *FramebufferImage::pixel(spk::Vector2UInt position) const
	{
		if (position.x >= size.x || position.y >= size.y)
		{
			throw std::out_of_range("Framebuffer pixel position is out of range");
		}
		const std::size_t index =
			(static_cast<std::size_t>(position.y) * size.x + position.x) * 4;
		return pixels.data() + index;
	}

	OpenGLTestContext::OpenGLTestContext() :
		_impl(std::make_unique<Impl>())
	{
		reset();
	}

	OpenGLTestContext::~OpenGLTestContext() = default;

	OpenGLTestContext &OpenGLTestContext::instance()
	{
		static OpenGLTestContext result;
		return result;
	}

	void OpenGLTestContext::makeCurrent()
	{
		_impl->requireOwnerThread();
		_impl->surface.makeCurrent();
	}

	void OpenGLTestContext::reset()
	{
		makeCurrent();
		// OpenGL errors belong to the context. Clear errors deliberately produced
		// by an earlier test when this fixture is shared by one test process.
		while (::glGetError() != GL_NO_ERROR)
		{
		}
		_impl->surface.setGeometry({.anchor = {0, 0}, .size = {FramebufferWidth, FramebufferHeight}});
		_impl->framebuffer.activate(_impl->renderContext);
		::glDrawBuffer(GL_COLOR_ATTACHMENT0);
		::glReadBuffer(GL_COLOR_ATTACHMENT0);
		::glUseProgram(0);
		::glBindVertexArray(0);
		::glActiveTexture(GL_TEXTURE0);
		::glBindTexture(GL_TEXTURE_2D, 0);
		::glDisable(GL_SCISSOR_TEST);
		::glEnable(GL_BLEND);
		::glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		::glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		::glEnable(GL_DEPTH_TEST);
		::glDepthFunc(GL_LEQUAL);
		::glDepthMask(GL_TRUE);
		::glEnable(GL_CULL_FACE);
		::glFrontFace(GL_CCW);
		::glCullFace(GL_BACK);
		::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		::glStencilMask(~0u);
		::glViewport(0, 0, static_cast<GLsizei>(FramebufferWidth), static_cast<GLsizei>(FramebufferHeight));
		::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		::glClearDepth(1.0);
		::glClearStencil(0);
		::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		if (const GLenum error = ::glGetError(); error != GL_NO_ERROR)
		{
			throw std::runtime_error("OpenGL test-context reset failed with error " + std::to_string(error));
		}
	}

	void OpenGLTestContext::setGeometry(const spk::Rect2D &geometry)
	{
		makeCurrent();
		_impl->surface.setGeometry(geometry);
	}

	spk::RenderContext &OpenGLTestContext::renderContext()
	{
		makeCurrent();
		return _impl->renderContext;
	}

	spk::Window::Surface &OpenGLTestContext::surface()
	{
		makeCurrent();
		return _impl->surface;
	}

	FramebufferImage OpenGLTestContext::capture()
	{
		makeCurrent();
		const spk::Vector2UInt size = _impl->surface.geometry().size;
		FramebufferImage result{
			.size = size,
			.pixels = std::vector<std::uint8_t>(static_cast<std::size_t>(size.x) * size.y * 4)};

		::glFinish();
		::glPixelStorei(GL_PACK_ALIGNMENT, 1);
		::glReadPixels(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y), GL_RGBA, GL_UNSIGNED_BYTE, result.pixels.data());

		const std::size_t rowSize = static_cast<std::size_t>(size.x) * 4;
		for (std::uint32_t y = 0; y < size.y / 2; ++y)
		{
			auto top = result.pixels.begin() + static_cast<std::ptrdiff_t>(y * rowSize);
			auto bottom = result.pixels.begin() + static_cast<std::ptrdiff_t>((size.y - y - 1) * rowSize);
			std::swap_ranges(top, top + static_cast<std::ptrdiff_t>(rowSize), bottom);
		}
		return result;
	}

	void OpenGLTestContext::save(const std::filesystem::path &path)
	{
		const FramebufferImage image = capture();
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}
		if (::stbi_write_png(
				path.string().c_str(),
				static_cast<int>(image.size.x),
				static_cast<int>(image.size.y),
				4,
				image.pixels.data(),
				static_cast<int>(image.size.x * 4)) == 0)
		{
			throw std::runtime_error("Failed to write OpenGL framebuffer image [" + path.string() + "]");
		}
	}
}
