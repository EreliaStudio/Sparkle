#include "core/platform/window.hpp"
#include "core/window.hpp"

#include <atomic>
#include <utility>

namespace spk
{
	struct Window::Native::Impl
	{
		Window::Identifier windowID;
		std::atomic<LifeCycle> lifeCycle = LifeCycle::Pending;
		WinAPI::Window window;

		explicit Impl(Window::Identifier windowID) :
			windowID(std::move(windowID))
		{
		}
	};

	Window::Native::Native(const Window::Identifier &windowID) :
		_impl(std::make_unique<Impl>(windowID))
	{
	}
	Window::Native::~Native() = default;

	Window::LifeCycle Window::Native::lifeCycle() const noexcept
	{
		return _impl->lifeCycle.load();
	}

	WinAPI::Window &Window::Native::window() noexcept
	{
		return _impl->window;
	}

	const WinAPI::Window &Window::Native::window() const noexcept
	{
		return _impl->window;
	}

	void Window::Native::markReady() noexcept
	{
		_impl->lifeCycle = LifeCycle::Ready;
	}

	void Window::Native::beginRelease() noexcept
	{
		_impl->lifeCycle = LifeCycle::Releasing;
	}

	void Window::Native::markReleased() noexcept
	{
		_impl->lifeCycle = LifeCycle::Released;
	}
}
