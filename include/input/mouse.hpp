#pragma once

#include <array>
#include <cstddef>

#include "input/input_state.hpp"
#include "math/vector2.hpp"

namespace spk
{
	struct Mouse
	{
		enum Button
		{
			Right,
			Middle,
			Left
		};
		static inline constexpr std::size_t NbButton = 3;

		std::array<InputState, NbButton> buttons{};
		spk::Vector2Int position{0, 0};
		spk::Vector2Int deltaPosition{0, 0};
		float wheel = 0;

		Mouse();
		InputState &operator[](Button button);
		[[nodiscard]] const InputState &operator[](Button button) const;
	};
}
