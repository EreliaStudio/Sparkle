#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ui/layout/linear_layout.hpp"
#include "ui/widget/push_button.hpp"
#include "ui/widget/spacer_widget.hpp"

namespace spk
{
	class CommandPanel : public Widget
	{
	private:
		bool _layoutReady = false;
		HorizontalLayout _layout;
		SpacerWidget _spacer;
		std::unordered_map<std::string, std::unique_ptr<PushButton>> _buttons;
		std::vector<std::string> _buttonOrder;
		Layout::SizeSettings _sizePolicy{Layout::SizePolicy::Minimum, Layout::SizePolicy::Minimum};

		void _rebuildLayout();
		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit CommandPanel(std::string name, Widget *parent = nullptr);

		PushButton &addButton(std::string name, std::string_view label);
		PushButton &button(std::string_view name);
		[[nodiscard]] const PushButton &button(std::string_view name) const;
		void removeButton(std::string_view name);
		[[nodiscard]] std::size_t nbButton() const noexcept;
		[[nodiscard]] PushButton::ClickContract subscribe(std::string_view name, PushButton::ClickCallback callback);

		void setSizePolicy(Layout::SizeSettings sizePolicy);
		[[nodiscard]] const Layout::SizeSettings &sizePolicy() const noexcept;
		void setElementPadding(const Vector2UInt &padding);
		[[nodiscard]] const Vector2UInt &elementPadding() const noexcept;
	};
}
