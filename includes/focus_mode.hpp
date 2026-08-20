#pragma once

#include <array>

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
			Widget* widget;
		};

		std::array<Widget *, ChannelCount> targetWidgets{};
	};
}