#include "input/spk_input_state.hpp"

namespace spk
{
	std::ostream& operator << (std::ostream& p_os, const InputState& p_state)
	{
		switch (p_state)
		{
			case InputState::Pressed:
				p_os << "Pressed"; break;
			case InputState::Down:
				p_os << "Down"; break;
			case InputState::Released:
				p_os << "Released"; break;
			case InputState::Up:
				p_os << "Up"; break;
			default:
				p_os << "Unknow state"; break;
		}
		return (p_os);
	}
}