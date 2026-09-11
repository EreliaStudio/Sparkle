#pragma once

#include <array>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace spk
{
	class Widget;

	struct FocusMode
	{
		enum class Channel
		{
			Keyboard = 0,
			Mouse = 1
		};
		static inline constexpr std::size_t ChannelCount = 2;

		enum class ChangeType
		{
			Take,
			Release
		};

		struct Record
		{
			ChangeType type;
			Widget *widget;
		};

		std::array<Widget *, ChannelCount> targetWidgets{};
	};

	[[nodiscard]] constexpr std::string_view toString(FocusMode::Channel channel)
	{
		switch (channel)
		{
		case FocusMode::Channel::Keyboard:
			return "Keyboard";
		case FocusMode::Channel::Mouse:
			return "Mouse";
		}

		return "Unknown";
	}

	[[nodiscard]] constexpr std::wstring_view toWString(FocusMode::Channel channel)
	{
		switch (channel)
		{
		case FocusMode::Channel::Keyboard:
			return L"Keyboard";
		case FocusMode::Channel::Mouse:
			return L"Mouse";
		}

		return L"Unknown";
	}

	[[nodiscard]] constexpr std::string_view toString(FocusMode::ChangeType type)
	{
		switch (type)
		{
		case FocusMode::ChangeType::Take:
			return "Take";
		case FocusMode::ChangeType::Release:
			return "Release";
		}

		return "Unknown";
	}

	[[nodiscard]] constexpr std::wstring_view toWString(FocusMode::ChangeType type)
	{
		switch (type)
		{
		case FocusMode::ChangeType::Take:
			return L"Take";
		case FocusMode::ChangeType::Release:
			return L"Release";
		}

		return L"Unknown";
	}

	inline std::ostream &operator<<(std::ostream &stream, FocusMode::Channel channel)
	{
		return stream << toString(channel);
	}

	inline std::wostream &operator<<(std::wostream &stream, FocusMode::Channel channel)
	{
		return stream << toWString(channel);
	}

	inline std::ostream &operator<<(std::ostream &stream, FocusMode::ChangeType type)
	{
		return stream << toString(type);
	}

	inline std::wostream &operator<<(std::wostream &stream, FocusMode::ChangeType type)
	{
		return stream << toWString(type);
	}

	inline std::ostream &operator<<(std::ostream &stream, const FocusMode::Record &record)
	{
		return stream << '(' << record.type << ", " << static_cast<const void *>(record.widget) << ')';
	}

	inline std::wostream &operator<<(std::wostream &stream, const FocusMode::Record &record)
	{
		return stream << L'(' << record.type << L", " << static_cast<const void *>(record.widget) << L')';
	}

	[[nodiscard]] inline std::string toString(const FocusMode::Record &record)
	{
		std::ostringstream stream;
		stream << record;
		return stream.str();
	}

	[[nodiscard]] inline std::wstring toWString(const FocusMode::Record &record)
	{
		std::wostringstream stream;
		stream << record;
		return stream.str();
	}

	inline std::ostream &operator<<(std::ostream &stream, const FocusMode &mode)
	{
		stream << '(';
		for (std::size_t index = 0; index < FocusMode::ChannelCount; ++index)
		{
			if (index != 0)
			{
				stream << ", ";
			}
			stream << static_cast<const void *>(mode.targetWidgets[index]);
		}
		return stream << ')';
	}

	inline std::wostream &operator<<(std::wostream &stream, const FocusMode &mode)
	{
		stream << L'(';
		for (std::size_t index = 0; index < FocusMode::ChannelCount; ++index)
		{
			if (index != 0)
			{
				stream << L", ";
			}
			stream << static_cast<const void *>(mode.targetWidgets[index]);
		}
		return stream << L')';
	}

	[[nodiscard]] inline std::string toString(const FocusMode &mode)
	{
		std::ostringstream stream;
		stream << mode;
		return stream.str();
	}

	[[nodiscard]] inline std::wstring toWString(const FocusMode &mode)
	{
		std::wostringstream stream;
		stream << mode;
		return stream.str();
	}
}
