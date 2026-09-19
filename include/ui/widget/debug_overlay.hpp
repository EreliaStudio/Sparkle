#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>

#include "ui/layout/grid_layout.hpp"
#include "ui/widget.hpp"

namespace spk
{
	class DebugOverlay : public Widget
	{
	private:
		using ParentEditionContract = InherenceTrait<Widget, WidgetChildComparator>::OnParentEditionContract;

		GridLayout _layout;
		ResizeableTrait::Contract _layoutSizeContract;
		std::unordered_map<Widget *, ParentEditionContract> _widgetParentContracts;
		bool _layoutReady = false;

		void _forgetWidget(Widget *widget);
		[[nodiscard]] bool _containsWidget(const Widget *widget) const noexcept;
		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit DebugOverlay(std::string name, Widget *parent = nullptr);
		~DebugOverlay() override;

		Layout::Element *setWidget(
			std::size_t column,
			std::size_t row,
			Widget *widget,
			Layout::SizeSettings sizeSettings = {});
		void clearCell(std::size_t column, std::size_t row);
		void clear();

		void setElementPadding(const Vector2UInt &padding);
		[[nodiscard]] const Vector2UInt &elementPadding() const noexcept;
		[[nodiscard]] std::size_t rowCount() const noexcept;
		[[nodiscard]] std::size_t columnCount() const noexcept;
		[[nodiscard]] Widget *widget(std::size_t column, std::size_t row) noexcept;
		[[nodiscard]] const Widget *widget(std::size_t column, std::size_t row) const noexcept;
	};
}
