#pragma once

#include "widget/spk_widget.hpp"
#include "widget/widget_component/spk_nine_sliced_component.hpp"
#include "widget/widget_component/spk_text_label_component.hpp"

namespace spk
{
	class Button : public spk::IWidget
	{
	public:
		using Callback = std::function<void()>;

	private:
		Callback _callback;

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
			if (spk::Application::activeApplication()->mouse().getButton(spk::Mouse::Left) == spk::InputState::Pressed &&
				isPointed(spk::Application::activeApplication()->mouse().position()) == true)
			{
				_callback();
			}
		}

		bool isPointed(const spk::Vector2Int& p_point)
		{
			return (spk::Vector2Int::isInsideRectangle(p_point, viewport().anchor(), viewport().anchor() + viewport().size()));
		}

	public:
		Button(const std::string& p_name, const Callback& p_callback) :
			spk::IWidget(p_name),
			_callback(p_callback)
		{

		}

		spk::WidgetComponent::NineSlicedBox& box() {return (_box);}
		spk::WidgetComponent::TextLabel& label() {return (_label);}
	};
}