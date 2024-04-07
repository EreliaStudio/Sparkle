#pragma once

#include "widget/spk_widget.hpp"
#include "widget/components/spk_nine_sliced.hpp"

namespace spk
{
	class Frame : public spk::Widget
	{
	private:
		spk::WidgetComponent::NineSlicedBox _box;

		void _onGeometryChange();
		void _onRender();

	public:
		Frame(spk::Widget* p_parent = nullptr);

		spk::WidgetComponent::NineSlicedBox& box();
		const spk::WidgetComponent::NineSlicedBox& box() const;
	};
}