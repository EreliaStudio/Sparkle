#include "structure/system/device/spk_mouse.hpp"

std::ostream& operator << (std::ostream& p_os, const spk::Mouse::Button& p_button)
{
	switch (p_button)
	{
	case spk::Mouse::Button::Left:
		p_os << "Left"; break;
	case spk::Mouse::Button::Right:
		p_os << "Right"; break;
	case spk::Mouse::Button::Middle:
		p_os << "Middle"; break;
	}
	return (p_os);
}

std::wostream& operator << (std::wostream& p_os, const spk::Mouse::Button& p_button)
{
	switch (p_button)
	{
	case spk::Mouse::Button::Left:
		p_os << "Left"; break;
	case spk::Mouse::Button::Right:
		p_os << "Right"; break;
	case spk::Mouse::Button::Middle:
		p_os << "Middle"; break;
	}
	return (p_os);
}