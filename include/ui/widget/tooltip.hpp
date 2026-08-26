#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include "ui/widget/panel.hpp"
#include "ui/widget/text_area.hpp"

namespace spk
{
	class Tooltip : public Widget
	{
	public:
		using Duration = std::chrono::steady_clock::duration;
		enum class Placement
		{
			Cursor,
			AboveTarget,
			BelowTarget,
			Automatic
		};

	private:
		Panel _background;
		TextArea _textArea;
		Widget *_target = nullptr;
		Duration _openDelay = std::chrono::milliseconds(500);
		Duration _closeDelay{};
		Duration _hoverElapsed{};
		Duration _leaveElapsed{};
		Placement _placement = Placement::Automatic;
		Vector2Int _cursorOffset{12, 16};
		Vector2Int _lastCursor{};
		unsigned int _maximumWidth = 320;
		bool _shown = false;
		bool _hovering = false;
		Widget *_coordinatorRoot = nullptr;
		InherenceTrait<Widget, WidgetChildComparator>::OnParentEditionContract _targetParentContract;

		void _updateGeometry();
		void _updateState(UpdateContext &context) override;
		void _onMouseMovedEvent(MouseMovedEvent &event) override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onWindowFocusLostEvent(WindowFocusLostEvent &event) override;
		void _onPassiveMouseMovedEvent(MouseMovedEvent &event) override;
		void _onPassiveMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;

	public:
		explicit Tooltip(std::string name, Widget *parent = nullptr);
		~Tooltip() override;
		void applyStyle(const Style &style) override;
		void setTarget(Widget *target);
		void setText(Font::Text text);
		void setText(std::string_view text);
		void setOpenDelay(Duration duration);
		void setCloseDelay(Duration duration);
		void setPlacement(Placement placement);
		void setCursorOffset(const Vector2Int &offset);
		void setMaximumWidth(unsigned int width);
		void show();
		void hide();
		[[nodiscard]] bool isShown() const noexcept;
		[[nodiscard]] Widget *target() noexcept;
		[[nodiscard]] const Widget *target() const noexcept;
		[[nodiscard]] Duration openDelay() const noexcept;
		[[nodiscard]] Duration closeDelay() const noexcept;
		[[nodiscard]] Panel &background() noexcept;
		[[nodiscard]] const Panel &background() const noexcept;
		[[nodiscard]] TextArea &textArea() noexcept;
		[[nodiscard]] const TextArea &textArea() const noexcept;
	};
}
