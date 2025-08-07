#include "structure/system/device/spk_mouse.hpp"

#include "structure/graphics/spk_window.hpp"

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

namespace spk
{
	Mouse::Mouse()
	{
		for (size_t i = 0; i < 3; i++)
		{
			buttons[i] = InputState::Up;
		}
		position = {0, 0};
		deltaPosition = {0, 0};
		wheel = 0;
	}

	InputState Mouse::operator[](Mouse::Button p_button) const
	{
		return (buttons[static_cast<int>(p_button)]);
	}

	void Mouse::place(const SafePointer<Window>& p_window, const Vector2Int& p_newPosition) const
	{
		p_window->requestMousePlacement(p_newPosition);
	}
}