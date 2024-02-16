#pragma once

#include "widget/spk_widget.hpp"
#include "widget/widget_component/spk_nine_sliced_component.hpp"
#include "widget/widget_component/spk_text_label_component.hpp"

namespace spk
{
	class TextLabel : public spk::IWidget
	{
	private:
		spk::WidgetComponent::NineSlicedBox _box;
		spk::WidgetComponent::TextLabel _label;

		void _onGeometryChange()
		{
			_box.setGeometry(anchor(), size());
			_box.setDepth(depth());
			
			_label.setAnchor(anchor() + (size() / 2));
			_label.setDepth(depth() + 1);
		}

		void _onRender()
		{
			_box.render();
			_label.render();
		}

		void _onUpdate()
		{

		}

	public:
		TextLabel(const std::string& p_name) :
			spk::IWidget(p_name)
		{

		}

		spk::WidgetComponent::NineSlicedBox& box() {return (_box);}
		spk::WidgetComponent::TextLabel& label() {return (_label);}
	};
}