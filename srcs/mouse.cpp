#include "mouse.hpp"

namespace spk
{
	Mouse::Mouse()
	{
		buttons.fill(InputState::Up);
	}

	InputState &Mouse::operator[](Button button)
	{
		return buttons[static_cast<std::size_t>(button)];
	}

	const InputState &Mouse::operator[](Button button) const
	{
		return buttons[static_cast<std::size_t>(button)];
	}
}
