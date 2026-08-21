#include "window.hpp"

#include <atomic>
#include <utility>

#include "frame.hpp"

namespace spk
{
	struct Window::Native::Impl
	{
		Window::Identifier windowID;
		std::atomic<LifeCycle> lifeCycle = LifeCycle::Pending;
		WinAPI::Frame frame;

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

	WinAPI::Frame &Window::Native::frame() noexcept
	{
		return _impl->frame;
	}

	const WinAPI::Frame &Window::Native::frame() const noexcept
	{
		return _impl->frame;
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
