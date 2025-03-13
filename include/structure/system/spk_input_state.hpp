#pragma once

#include <iostream>

#include "spk_debug_macro.hpp"

namespace spk
{
	enum class InputState
	{
		Down,
		Up
	};
	
	inline std::ostream &operator<<(std::ostream &p_os, const spk::InputState &p_state)
	{
		switch (p_state)
		{
		case spk::InputState::Down:
			p_os << "Down";
			break;
		case spk::InputState::Up:
			p_os << "Up";
			break;
		default:
			p_os << "Unknow input state";
			break;
		}
		return p_os;
	}

	inline std::wostream &operator<<(std::wostream &p_os, const spk::InputState &p_state)
	{
		switch (p_state)
		{
		case spk::InputState::Down:
			p_os << L"Down";
			break;
		case spk::InputState::Up:
			p_os << L"Up";
			break;
		default:
			p_os << L"Unknow input state";
			break;
		}
		return p_os;
	}
}
