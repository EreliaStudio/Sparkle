#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "ui/layout/linear_layout.hpp"
#include "ui/widget/command_panel.hpp"
#include "ui/widget/panel.hpp"
#include "ui/widget/text_area.hpp"

namespace spk
{
	class PromptPanel : public Widget
	{
	private:
		bool _layoutReady = false;
		Panel _background;
		TextArea _textArea;
		CommandPanel _commandPanel;
		VerticalLayout _layout;
		ResizeableTrait::Contract _textSizeContract;
		ResizeableTrait::Contract _commandSizeContract;

		void _rebuildLayout();
		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit PromptPanel(std::string name, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		void setMessage(Font::Text message);
		void setMessage(std::string_view message);
		[[nodiscard]] const Font::Text &message() const noexcept;

		PushButton &addButton(std::string name, std::string_view label);
		PushButton &button(std::string_view name);
		[[nodiscard]] const PushButton &button(std::string_view name) const;
		void removeButton(std::string_view name);
		[[nodiscard]] std::size_t nbButton() const noexcept;
		[[nodiscard]] PushButton::ClickContract subscribe(std::string_view name, PushButton::ClickCallback callback);
		void setButtonSizePolicy(Layout::SizeSettings sizePolicy);
		[[nodiscard]] const Layout::SizeSettings &buttonSizePolicy() const noexcept;
		void setButtonPadding(const Vector2UInt &padding);
		[[nodiscard]] const Vector2UInt &buttonPadding() const noexcept;

		[[nodiscard]] Panel &background() noexcept;
		[[nodiscard]] const Panel &background() const noexcept;
		[[nodiscard]] TextArea &textArea() noexcept;
		[[nodiscard]] const TextArea &textArea() const noexcept;
		[[nodiscard]] CommandPanel &commandPanel() noexcept;
		[[nodiscard]] const CommandPanel &commandPanel() const noexcept;
	};
}
