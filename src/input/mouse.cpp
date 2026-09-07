#include "input/mouse.hpp"

namespace spk
{
	Mouse::Mouse()
	{
		buttons.fill(InputState::Up);
	}

	InputState &Mouse::operator[](Button button)
	{
		return buttons.at(static_cast<std::size_t>(button));
	}

	const InputState &Mouse::operator[](Button button) const
	{
		return buttons.at(static_cast<std::size_t>(button));
	}
}
