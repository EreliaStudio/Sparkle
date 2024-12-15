#pragma once

#include "widget/spk_widget.hpp"

#include "widget/component/spk_color_box.hpp"
#include "widget/component/spk_nine_slice_renderer.hpp"

namespace spk
{
	class Frame : public spk::Widget
	{
	private:
		spk::WidgetComponent::ColorBox _colorBoxComponent;
        spk::WidgetComponent::NineSliceRenderer _nineSliceComponent;

		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent& p_event) override;

	public:
		Frame(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);

		spk::WidgetComponent::ColorBox& colorBoxComponent();
		const spk::WidgetComponent::ColorBox& colorBoxComponent() const;
		
        spk::WidgetComponent::NineSliceRenderer& nineSliceComponent();
        const spk::WidgetComponent::NineSliceRenderer& nineSliceComponent() const;
	};
}