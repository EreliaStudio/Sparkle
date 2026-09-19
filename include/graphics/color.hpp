#pragma once

namespace spk
{
	struct Color
	{
		using value_type = float;

		value_type r = 0.0f;
		value_type g = 0.0f;
		value_type b = 0.0f;
		value_type a = 1.0f;

		[[nodiscard]] bool operator==(const Color &other) const noexcept = default;
	};
}