#include "window.hpp"

#include <utility>

#include "widget.hpp"

namespace spk
{
	Window::Window(std::shared_ptr<Native> native, std::shared_ptr<State> state, std::shared_ptr<Surface> surface) :
		_native(std::move(native)),
		_state(std::move(state)),
		_surface(std::move(surface))
	{
	}

	bool Window::isClosing() const noexcept
	{
		if (isClosed())
		{
			return false;
		}

		const auto hasStartedRelease = [](LifeCycle lifeCycle) {
			return lifeCycle == LifeCycle::Releasing || lifeCycle == LifeCycle::Released;
		};
		return hasStartedRelease(_native->lifeCycle()) || hasStartedRelease(_state->lifeCycle()) || hasStartedRelease(_surface->lifeCycle());
	}

	bool Window::isClosed() const noexcept
	{
		return _native->lifeCycle() == LifeCycle::Released &&
			   _state->lifeCycle() == LifeCycle::Released &&
			   _surface->lifeCycle() == LifeCycle::Released;
	}

	Widget &Window::root() noexcept
	{
		return _state->root();
	}

	const Widget &Window::root() const noexcept
	{
		return _state->root();
	}

	const spk::Rect2D &Window::geometry() const noexcept
	{
		return _surface->geometry();
	}
}
