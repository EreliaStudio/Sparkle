#include "window.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

#include "keyboard.hpp"
#include "mouse.hpp"
#include "widget.hpp"

#include "clear_render_command.hpp"

namespace spk
{
	class RootWidget : public Widget
	{
	private:
		spk::Color _backgroundColor{
			.r = 0,
			.g = 0,
			.b = 0,
			.a = 1};

		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
		{
			auto &pass = builder.renderPass(Widget::OverlayKey);

			pass.emplace<spk::ClearRenderCommand>(
				_backgroundColor,
				spk::ClearRenderCommand::Mask::All);
		}

	public:
		RootWidget(const std::string &name, Widget *parent) :
			Widget(name, parent)
		{
		}

		void setBackgroundColor(const spk::Color &backgroundColor)
		{
			_backgroundColor = backgroundColor;
		}
	};
	struct Window::State::Impl
	{
		Window::Identifier windowID;
		std::atomic<LifeCycle> lifeCycle = LifeCycle::Pending;
		std::unique_ptr<RootWidget> root;
		std::array<Widget *, FocusMode::ChannelCount> focusedWidgets{};
		spk::Keyboard keyboard;
		spk::Mouse mouse;

		explicit Impl(Window::Identifier windowID) :
			windowID(std::move(windowID)),
			root(std::make_unique<RootWidget>("/Root widget", nullptr))
		{
			root->activate();
		}
	};

	Window::State::State(const Window::Identifier &windowID) :
		_impl(std::make_unique<Impl>(windowID))
	{
	}
	Window::State::~State() = default;

	Window::LifeCycle Window::State::lifeCycle() const noexcept
	{
		return _impl->lifeCycle.load();
	}

	Widget *Window::State::focusedWidget(FocusMode::Channel channel) noexcept
	{
		return _impl->focusedWidgets[static_cast<std::size_t>(channel)];
	}

	const Widget *Window::State::focusedWidget(FocusMode::Channel channel) const noexcept
	{
		return _impl->focusedWidgets[static_cast<std::size_t>(channel)];
	}

	void Window::State::setBackgroundColor(const spk::Color &backgroundColor)
	{
		_impl->root->setBackgroundColor(backgroundColor);
	}

	void Window::State::takeFocus(FocusMode::Channel channel, Widget *widget) noexcept
	{
		if (widget != nullptr)
		{
			_impl->focusedWidgets[static_cast<std::size_t>(channel)] = widget;
		}
	}

	void Window::State::releaseFocus(FocusMode::Channel channel, Widget *widget) noexcept
	{
		Widget *&owner = _impl->focusedWidgets[static_cast<std::size_t>(channel)];
		if (owner == widget)
		{
			owner = nullptr;
		}
	}

	void Window::State::clearFocus(FocusMode::Channel channel) noexcept
	{
		_impl->focusedWidgets[static_cast<std::size_t>(channel)] = nullptr;
	}

	Widget &Window::State::dispatchRoot(FocusMode::Channel channel) noexcept
	{
		Widget *focused = focusedWidget(channel);
		return focused != nullptr ? *focused : *_impl->root;
	}

	void Window::State::markReady() noexcept
	{
		_impl->lifeCycle = LifeCycle::Ready;
	}
	void Window::State::beginRelease() noexcept
	{
		_impl->lifeCycle = LifeCycle::Releasing;
	}
	void Window::State::markReleased() noexcept
	{
		_impl->lifeCycle = LifeCycle::Released;
	}
	Widget &Window::State::root() noexcept
	{
		return *_impl->root;
	}
	const Widget &Window::State::root() const noexcept
	{
		return *_impl->root;
	}
	spk::Keyboard &Window::State::keyboard() noexcept
	{
		return _impl->keyboard;
	}
	const spk::Keyboard &Window::State::keyboard() const noexcept
	{
		return _impl->keyboard;
	}
	spk::Mouse &Window::State::mouse() noexcept
	{
		return _impl->mouse;
	}
	const spk::Mouse &Window::State::mouse() const noexcept
	{
		return _impl->mouse;
	}
}
