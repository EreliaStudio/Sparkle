#pragma once

#include "widget/spk_widget.hpp"
#include "widget/spk_action_bar.hpp"

namespace spk
{
	template<typename TContent>
	class HUDWidget : public spk::Widget
	{
	private:
		spk::MenuBar _menuBar;
		TContent _content;

		void _onGeometryChange()
		{
			_menuBar.setGeometry(geometry());
			_content.setGeometry({0, _menuBar.height()}, {geometry().size.x, geometry().size.y - _menuBar.height()});
		}

	public:
		HUDWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent),
			_menuBar(p_name + L" - MenuBar", this),
			_content(p_name, this)
		{
			_content.activate();
			_menuBar.activate();
		}

		spk::SafePointer<spk::MenuBar> menuBar()
		{
			return (&_menuBar);
		}

		spk::SafePointer<TContent> content()
		{
			return (&_content);
		}
	};
}