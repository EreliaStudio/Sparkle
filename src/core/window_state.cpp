#include "core/window.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

#include "input/keyboard.hpp"
#include "input/mouse.hpp"
#include "ui/widget.hpp"

#include "rendering/command/clear_render_command.hpp"

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
			builder.renderPass(Widget::BackgroundKey).emplace<spk::ClearRenderCommand>(_backgroundColor, spk::ClearRenderCommand::Mask::All);
			builder.renderPass(Widget::OverlayKey).emplace<spk::ClearRenderCommand>(spk::Color{}, spk::ClearRenderCommand::Mask::Depth);
			builder.renderPass(Widget::PopupKey).emplace<spk::ClearRenderCommand>(spk::Color{}, spk::ClearRenderCommand::Mask::Depth);
			builder.renderPass(Widget::TooltipKey).emplace<spk::ClearRenderCommand>(spk::Color{}, spk::ClearRenderCommand::Mask::Depth);
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
		std::array<ActivableTrait::DeactivationContract, FocusMode::ChannelCount> focusedWidgetDeactivationContracts{};
		std::array<Widget::DestructionContract, FocusMode::ChannelCount> focusedWidgetDestructionContracts{};
		spk::Keyboard keyboard;
		spk::Mouse mouse;

		explicit Impl(Window::Identifier windowID) :
			windowID(std::move(windowID)),
			root(std::make_unique<RootWidget>("/Root widget", nullptr))
		{
			root->activate();
		}

		void forgetFocus(std::size_t index, Widget *expected, bool notify) noexcept
		{
			Widget *&owner = focusedWidgets[index];
			if (owner != expected)
			{
				return;
			}
			owner = nullptr;
			focusedWidgetDeactivationContracts[index] = {};
			focusedWidgetDestructionContracts[index] = {};
			if (notify && expected != nullptr)
			{
				expected->notifyFocusReleased(static_cast<FocusMode::Channel>(index));
			}
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

	void Window::State::takeFocus(FocusMode::Channel channel, Widget *widget)
	{
		if (widget == nullptr)
		{
			return;
		}
		const std::size_t index = static_cast<std::size_t>(channel);
		Widget *&owner = _impl->focusedWidgets[index];
		if (owner == widget)
		{
			return;
		}
		if (owner != nullptr)
		{
			_impl->forgetFocus(index, owner, true);
		}
		owner = widget;
		try
		{
			_impl->focusedWidgetDeactivationContracts[index] = widget->subscribeToDeactivation([impl = _impl.get(), index, widget]() {
				impl->forgetFocus(index, widget, true);
			});
			_impl->focusedWidgetDestructionContracts[index] = widget->subscribeToDestruction([impl = _impl.get(), index, widget](Widget *) {
				impl->forgetFocus(index, widget, false);
			});
		} catch (...)
		{
			_impl->forgetFocus(index, widget, false);
			throw;
		}
		owner->notifyFocusAcquired(channel);
	}

	void Window::State::releaseFocus(FocusMode::Channel channel, Widget *widget) noexcept
	{
		const std::size_t index = static_cast<std::size_t>(channel);
		Widget *&owner = _impl->focusedWidgets[index];
		if (owner == widget)
		{
			_impl->forgetFocus(index, owner, true);
		}
	}

	void Window::State::clearFocus(FocusMode::Channel channel) noexcept
	{
		const std::size_t index = static_cast<std::size_t>(channel);
		Widget *&owner = _impl->focusedWidgets[index];
		if (owner != nullptr)
		{
			_impl->forgetFocus(index, owner, true);
		}
	}

	Widget &Window::State::dispatchRoot(FocusMode::Channel channel) noexcept
	{
		Widget *focused = focusedWidget(channel);
		if (focused != nullptr && !focused->resolveInHierarchy([](const Widget &widget) { return widget.isActive(); }))
		{
			_impl->forgetFocus(static_cast<std::size_t>(channel), focused, true);
			focused = nullptr;
		}
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
