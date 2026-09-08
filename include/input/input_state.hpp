#pragma once

#include <iosfwd>
#include <string>

namespace spk
{
	enum class InputState
	{
		Down,
		Up
	};

	[[nodiscard]] constexpr std::string_view toString(InputState status)
	{
		switch (status)
		{
		case InputState::Up:
			return "Up";
		case InputState::Down:
			return "Down";
		}

		return "Unknown";
	}

	[[nodiscard]] constexpr std::wstring_view toWString(InputState status)
	{
		switch (status)
		{
		case InputState::Up:
			return L"Up";
		case InputState::Down:
			return L"Down";
		}

		return L"Unknown";
	}

	inline std::ostream &operator<<(std::ostream &stream, InputState status)
	{
		return stream << toString(status);
	}

	inline std::wostream &operator<<(std::wostream &stream, InputState status)
	{
		return stream << toWString(status);
	}
}