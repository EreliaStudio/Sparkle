#include "structure/system/device/spk_mouse.hpp"

std::ostream &operator<<(std::ostream &p_os, const spk::Mouse::Button &p_button)
{
	switch (p_button)
	{
	case spk::Mouse::Button::Left:
		p_os << "Left";
		break;
	case spk::Mouse::Button::Right:
		p_os << "Right";
		break;
	case spk::Mouse::Button::Middle:
		p_os << "Middle";
		break;
	}
	return (p_os);
}

std::wostream &operator<<(std::wostream &p_os, const spk::Mouse::Button &p_button)
{
	switch (p_button)
	{
	case spk::Mouse::Button::Left:
		p_os << "Left";
		break;
	case spk::Mouse::Button::Right:
		p_os << "Right";
		break;
	case spk::Mouse::Button::Middle:
		p_os << "Middle";
		break;
	}
	return (p_os);
}
spk::Mouse::Mouse()
{
	for (size_t i = 0; i < 3; i++)
	{
		buttons[i] = spk::InputState::Up;
	}
	position = {0, 0};
	deltaPosition = {0, 0};
	wheel = 0;
}

spk::InputState spk::Mouse::operator[](spk::Mouse::Button p_button) const
{
	return (buttons[static_cast<int>(p_button)]);
}
