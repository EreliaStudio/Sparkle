#pragma once

#include <ostream>
#include <string>
#include <string_view>

namespace spk
{
	struct Alignment
	{
		enum class Horizontal
		{
			Left,
			Center,
			Right
		};

		enum class Vertical
		{
			Top,
			Center,
			Bottom
		};

		Horizontal horizontal = Horizontal::Left;
		Vertical vertical = Vertical::Top;

		[[nodiscard]] constexpr bool operator==(const Alignment &other) const noexcept = default;
	};

	[[nodiscard]] constexpr std::string_view toString(Alignment::Horizontal alignment)
	{
		switch (alignment)
		{
		case Alignment::Horizontal::Left:
			return "Left";
		case Alignment::Horizontal::Center:
			return "Center";
		case Alignment::Horizontal::Right:
			return "Right";
		}

		return "Unknown";
	}

	[[nodiscard]] constexpr std::wstring_view toWString(Alignment::Horizontal alignment)
	{
		switch (alignment)
		{
		case Alignment::Horizontal::Left:
			return L"Left";
		case Alignment::Horizontal::Center:
			return L"Center";
		case Alignment::Horizontal::Right:
			return L"Right";
		}

		return L"Unknown";
	}

	[[nodiscard]] constexpr std::string_view toString(Alignment::Vertical alignment)
	{
		switch (alignment)
		{
		case Alignment::Vertical::Top:
			return "Top";
		case Alignment::Vertical::Center:
			return "Center";
		case Alignment::Vertical::Bottom:
			return "Bottom";
		}

		return "Unknown";
	}

	[[nodiscard]] constexpr std::wstring_view toWString(Alignment::Vertical alignment)
	{
		switch (alignment)
		{
		case Alignment::Vertical::Top:
			return L"Top";
		case Alignment::Vertical::Center:
			return L"Center";
		case Alignment::Vertical::Bottom:
			return L"Bottom";
		}

		return L"Unknown";
	}

	inline std::ostream &operator<<(std::ostream &stream, Alignment::Horizontal alignment)
	{
		return stream << toString(alignment);
	}

	inline std::wostream &operator<<(std::wostream &stream, Alignment::Horizontal alignment)
	{
		return stream << toWString(alignment);
	}

	inline std::ostream &operator<<(std::ostream &stream, Alignment::Vertical alignment)
	{
		return stream << toString(alignment);
	}

	inline std::wostream &operator<<(std::wostream &stream, Alignment::Vertical alignment)
	{
		return stream << toWString(alignment);
	}

	inline std::ostream &operator<<(std::ostream &stream, const Alignment &alignment)
	{
		return stream << '(' << alignment.horizontal << ", " << alignment.vertical << ')';
	}

	inline std::wostream &operator<<(std::wostream &stream, const Alignment &alignment)
	{
		return stream << L'(' << alignment.horizontal << L", " << alignment.vertical << L')';
	}

	[[nodiscard]] inline std::string toString(const Alignment &alignment)
	{
		return "(" + std::string(toString(alignment.horizontal)) + ", " +
			   std::string(toString(alignment.vertical)) + ")";
	}

	[[nodiscard]] inline std::wstring toWString(const Alignment &alignment)
	{
		return L"(" + std::wstring(toWString(alignment.horizontal)) + L", " +
			   std::wstring(toWString(alignment.vertical)) + L")";
	}
}