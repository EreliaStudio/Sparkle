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
Mouse::Mouse() : _position({0, 0}), _deltaPosition({0, 0}), _wheel(0), _window(nullptr)
{
for (size_t i = 0; i < 3; i++)
{
_buttons[i] = InputState::Up;
}
}

InputState Mouse::operator[](Mouse::Button p_button) const
{
return (_buttons[static_cast<int>(p_button)]);
}

const Vector2Int &Mouse::position() const
{
return (_position);
}

const Vector2Int &Mouse::deltaPosition() const
{
return (_deltaPosition);
}

const InputState *Mouse::buttons() const
{
return (_buttons);
}

float Mouse::wheel() const
{
return (_wheel);
}

spk::SafePointer<Window> Mouse::window() const
{
return (_window);
}

void Mouse::place(const Vector2Int &p_newPosition) const
{
if (_window != nullptr)
{
_window->requestMousePlacement(p_newPosition);
}
}
}
