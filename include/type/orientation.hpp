#pragma once

namespace spk
{
	enum class Orientation
	{
		Vertical,
		Horizontal
	};

	[[nodiscard]] constexpr std::string_view toString(Orientation orientation)
	{
		switch (orientation)
		{
		case Orientation::Vertical:
			return "Vertical";
		case Orientation::Horizontal:
			return "Horizontal";
		}

		return "Unknown";
	}

	[[nodiscard]] constexpr std::wstring_view toWString(Orientation orientation)
	{
		switch (orientation)
		{
		case Orientation::Vertical:
			return L"Vertical";
		case Orientation::Horizontal:
			return L"Horizontal";
		}

		return L"Unknown";
	}

	inline std::ostream &operator<<(std::ostream &stream, Orientation orientation)
	{
		return stream << toString(orientation);
	}

	inline std::wostream &operator<<(std::wostream &stream, Orientation orientation)
	{
		return stream << toWString(orientation);
	}
}