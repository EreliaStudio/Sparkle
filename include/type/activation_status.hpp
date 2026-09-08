#pragma once

#include <string>
#include <iostream>

namespace spk
{
	enum class ActivationStatus
	{
		Activated,
		Deactivated
	};

	[[nodiscard]] constexpr std::string_view toString(ActivationStatus status)
	{
		switch (status)
		{
		case ActivationStatus::Activated:
			return "Activated";
		case ActivationStatus::Deactivated:
			return "Deactivated";
		}

		return "Unknown";
	}

	[[nodiscard]] constexpr std::wstring_view toWString(ActivationStatus status)
	{
		switch (status)
		{
		case ActivationStatus::Activated:
			return L"Activated";
		case ActivationStatus::Deactivated:
			return L"Deactivated";
		}

		return L"Unknown";
	}

	inline std::ostream &operator<<(std::ostream &stream, ActivationStatus status)
	{
		return stream << toString(status);
	}

	inline std::wostream &operator<<(std::wostream &stream, ActivationStatus status)
	{
		return stream << toWString(status);
	}
}
