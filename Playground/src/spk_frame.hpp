#pragma once

#include "widget/spk_widget.hpp"
#include "widget/widget_component/spk_nine_sliced_component.hpp"

namespace spk
{
	class Frame : public spk::IWidget
	{
	private:
		spk::WidgetComponent::NineSlicedBox _box;

		void _onGeometryChange()
		{
			_box.setGeometry(anchor(), size());
			_box.setDepth(depth());
		}

		void _onRender()
		{
			_box.render();
		}

		void _onUpdate()
		{
			
		}

	public:
		Frame(const std::string& p_name) :
			spk::IWidget(p_name)
		{

		}

		spk::WidgetComponent::NineSlicedBox& box() {return (_box);}
	};
}