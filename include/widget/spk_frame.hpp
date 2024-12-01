#pragma once

#include "widget/spk_widget.hpp"

#include "widget/component/spk_color_box.hpp"

namespace spk
{
	class Frame : public spk::Widget
	{
	private:
		spk::WidgetComponent::ColorBox _component;

		void _onGeometryChange() override;
		void _onPaintEvent(const spk::PaintEvent& p_event) override;

	public:
		Frame(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);
	};
}