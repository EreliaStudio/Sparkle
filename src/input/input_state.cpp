#include "input/input_state.hpp"

#include <ostream>

namespace spk
{
	std::string toString(const InputState &inputState)
	{
		switch (inputState)
		{
		case InputState::Down:
			return "Down";
		case InputState::Up:
			return "Up";
		}

		return "Unknow InputState";
	}

	std::wstring toWstring(const InputState &inputState)
	{
		switch (inputState)
		{
		case InputState::Down:
			return L"Down";
		case InputState::Up:
			return L"Up";
		}

		return L"Unknow InputState";
	}

	std::ostream &operator<<(std::ostream &stream, const InputState &inputState)
	{
		return stream << toString(inputState);
	}

	std::wostream &operator<<(std::wostream &stream, const InputState &inputState)
	{
		return stream << toWstring(inputState);
	}
}
