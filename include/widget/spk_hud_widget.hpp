#pragma once

#include "widget/spk_action_bar.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	template <typename TContent>
	class HUDWidget : public spk::Widget
	{
	private:
		spk::MenuBar _menuBar;
		spk::ContractProvider::Contract _menuBarActivationContract;
		TContent _content;

		void _onGeometryChange()
		{
			if (_menuBar.isActive() == true)
			{
				_menuBar.setGeometry(geometry());
				_content.setGeometry({0, _menuBar.height()}, {geometry().size.x, geometry().size.y - _menuBar.height()});
			}
			else
			{
				_content.setGeometry(geometry());
			}
		}

	public:
		HUDWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent),
			_menuBar(p_name + L" - MenuBar", this),
			_content(p_name, this)
		{
			_menuBarActivationContract = _menuBar.addActivationCallback([&]() { requireGeometryUpdate(); });

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