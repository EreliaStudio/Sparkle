#include "widget/spk_screen.hpp"

namespace spk
{
	Screen::Screen(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) : 
		spk::Widget(p_name, p_parent)
	{
		addActivationCallback([&](){
					if (_activeMenu != nullptr)
					{
						_activeMenu->deactivate();
					}
					_activeMenu = this;
				}).relinquish();
	}
}