#pragma once

namespace spk
{
	struct RGB
	{
		using value_type = float;

		value_type r = 0.0f;
		value_type g = 0.0f;
		value_type b = 0.0f;

		[[nodiscard]] bool operator==(const RGB &other) const noexcept = default;
	};

	struct RGBA
	{
		using value_type = float;

		value_type r = 0.0f;
		value_type g = 0.0f;
		value_type b = 0.0f;
		value_type a = 1.0f;

		[[nodiscard]] bool operator==(const RGBA &other) const noexcept = default;
	};

	using Color = RGBA;
}
