#pragma once

#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "design_pattern/contract_provider.hpp"
#include "ui/widget/icon_button.hpp"
#include "ui/widget/panel.hpp"
#include "ui/widget/scalable_widget.hpp"
#include "ui/widget/text_label.hpp"

namespace spk
{
	class IInterfaceWindow : public ScalableWidget
	{
	public:
		class MenuBar : public Widget
		{
		public:
			enum class Button
			{
				Minimize,
				Maximize,
				Close
			};

		private:
			bool _layoutReady = false;
			TextLabel _title;
			IconButton _minimizeButton;
			IconButton _maximizeButton;
			IconButton _closeButton;
			unsigned int _margin = 3;

			[[nodiscard]] IconButton &_button(Button button);
			[[nodiscard]] const IconButton &_button(Button button) const;
			void _updateSizeHint() override;
			void _onGeometryChange() override;

		public:
			explicit MenuBar(std::string name, Widget *parent = nullptr);

			void setTitle(std::string_view title);
			void setIconset(const SpriteSheet *iconset);
			void setButtonEnabled(Button button, bool enabled);
			void setMargin(unsigned int margin);

			[[nodiscard]] bool isButtonEnabled(Button button) const;
			[[nodiscard]] unsigned int margin() const noexcept;
			[[nodiscard]] TextLabel &titleLabel() noexcept;
			[[nodiscard]] const TextLabel &titleLabel() const noexcept;
			[[nodiscard]] IconButton &minimizeButton() noexcept;
			[[nodiscard]] const IconButton &minimizeButton() const noexcept;
			[[nodiscard]] IconButton &maximizeButton() noexcept;
			[[nodiscard]] const IconButton &maximizeButton() const noexcept;
			[[nodiscard]] IconButton &closeButton() noexcept;
			[[nodiscard]] const IconButton &closeButton() const noexcept;
		};

		struct Padding
		{
			unsigned int left = 0;
			unsigned int right = 0;
			unsigned int top = 0;
			unsigned int bottom = 0;

			bool operator==(const Padding &) const = default;
		};

		using ResizeProvider = ContractProvider<const Vector2UInt &>;
		using ResizeCallback = ResizeProvider::callback_type;
		using ResizeContract = ResizeProvider::Contract;
		using CloseCallback = PushButton::ClickCallback;
		using CloseContract = PushButton::ClickContract;

	private:
		bool _layoutReady = false;
		Panel _normalBackground;
		Panel _minimizedBackground;
		MenuBar _menuBar;
		Widget *_content = nullptr;
		std::optional<Padding> _contentPadding;
		Vector2 _minimumContentSize{};
		unsigned int _menuHeight = 20;
		bool _minimized = false;
		bool _maximized = false;
		bool _moving = false;
		Vector2Int _moveOffset{};
		Rect2D _restoredGeometry{};
		std::size_t _maximizeSpriteID = 1;
		std::size_t _restoreSpriteID = 2;
		Vector2UInt _lastContentSize{};
		InherenceTrait<Widget, WidgetChildComparator>::OnParentEditionContract _contentParentEditionContract;
		ResizeableTrait::Contract _contentSizeHintEditionContract;
		PushButton::ClickContract _minimizeContract;
		PushButton::ClickContract _maximizeContract;
		ResizeProvider _resizeProvider;

		[[nodiscard]] Padding _effectivePadding() const noexcept;
		[[nodiscard]] unsigned int _effectiveMenuHeight() const noexcept;
		[[nodiscard]] Vector2UInt _availableContentSize() const noexcept;
		[[nodiscard]] Rect2D _maximizedGeometry() const;
		void _applyMinimizedState();
		void _fitMaximizedGeometry();
		void _synchronizeMaximizedState();
		void _restore();
		void _beginMove(EventBase &event, Mouse::Button button, const Vector2Int &position);
		void _endMove(EventBase &event);
		void _updateSizeHint() override;
		void _updateState(UpdateContext &context) override;
		void _onGeometryChange() override;
		void _onWindowResizedEvent(WindowResizedEvent &event) override;
		void _onWindowFocusLostEvent(WindowFocusLostEvent &event) override;
		void _onMouseMovedEvent(MouseMovedEvent &event) override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event) override;
		void _onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &event) override;

	public:
		explicit IInterfaceWindow(std::string name, Widget *parent = nullptr);

		void setContent(Widget *content);
		void setTitle(std::string_view title);
		void setMenuHeight(unsigned int height);
		void setContentPadding(const Padding &padding);
		void resetContentPadding();
		void setMinimumContentSize(const Vector2 &size);
		void setMenuButtonEnabled(MenuBar::Button button, bool enabled);
		void setMaximizeSpriteIDs(std::size_t maximizeSpriteID, std::size_t restoreSpriteID);
		void minimize();
		void maximize();
		void close();

		[[nodiscard]] ResizeContract subscribeToResize(ResizeCallback callback);
		[[nodiscard]] CloseContract subscribeToClose(CloseCallback callback);
		[[nodiscard]] bool isMinimized() const noexcept;
		[[nodiscard]] bool isMaximized() const noexcept;
		[[nodiscard]] bool isMoving() const noexcept;
		[[nodiscard]] Widget *content() noexcept;
		[[nodiscard]] const Widget *content() const noexcept;
		[[nodiscard]] const std::optional<Padding> &contentPadding() const noexcept;
		[[nodiscard]] const Vector2 &minimumContentSize() const noexcept;
		[[nodiscard]] unsigned int menuHeight() const noexcept;
		[[nodiscard]] Panel &normalBackground() noexcept;
		[[nodiscard]] const Panel &normalBackground() const noexcept;
		[[nodiscard]] Panel &minimizedBackground() noexcept;
		[[nodiscard]] const Panel &minimizedBackground() const noexcept;
		[[nodiscard]] MenuBar &menuBar() noexcept;
		[[nodiscard]] const MenuBar &menuBar() const noexcept;
	};

	template <typename TContent>
	concept InterfaceWindowContent = std::derived_from<TContent, Widget> && std::constructible_from<TContent, std::string, Widget *>;

	template <InterfaceWindowContent TContent>
	class InterfaceWindow : public IInterfaceWindow
	{
	private:
		TContent _contentObject;

	public:
		explicit InterfaceWindow(std::string name, Widget *parent = nullptr) :
			IInterfaceWindow(std::move(name), parent),
			_contentObject(this->name() + ".content", &normalBackground())
		{
			setContent(&_contentObject);
		}

		[[nodiscard]] TContent &contentObject() noexcept
		{
			return _contentObject;
		}

		[[nodiscard]] const TContent &contentObject() const noexcept
		{
			return _contentObject;
		}
	};
}
