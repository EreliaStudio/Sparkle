#pragma once

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
}
