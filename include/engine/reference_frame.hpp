#pragma once

namespace spk
{
	enum class ReferenceFrame
	{
		Local,
		World
	};

	[[nodiscard]] constexpr std::string_view toString(ReferenceFrame frame)
	{
		switch (frame)
		{
		case ReferenceFrame::Local:
			return "Local";
		case ReferenceFrame::World:
			return "World";
		}

		return "Unknown";
	}

	[[nodiscard]] constexpr std::wstring_view toWString(ReferenceFrame frame)
	{
		switch (frame)
		{
		case ReferenceFrame::Local:
			return L"Local";
		case ReferenceFrame::World:
			return L"World";
		}

		return L"Unknown";
	}

	inline std::ostream &operator<<(std::ostream &stream, ReferenceFrame frame)
	{
		return stream << toString(frame);
	}

	inline std::wostream &operator<<(std::wostream &stream, ReferenceFrame frame)
	{
		return stream << toWString(frame);
	}
}
