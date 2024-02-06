#pragma once

#include "input/spk_input_state.hpp"
#include "math/spk_vector2.hpp"

namespace spk
{
	class Mouse
	{
	public:
		enum Button
		{
			Left,
			Middle,
			Right,
			Button3,
			Button4
		};

		static const size_t NB_BUTTON = 5;

	private:
		Vector2Int INITIAL_DELTA_POSITION = Vector2Int(-10000, -10000);
		InputState _buttons[NB_BUTTON];

		Vector2Int _position;
		Vector2Int _deltaPosition = INITIAL_DELTA_POSITION;
		Vector2Int _wheel;

	private:

	public:
		Mouse();
		~Mouse();

		void setMousePosition(const Vector2Int& p_newPosition);
		void pressButton(const Button& p_button);
		void releaseButton(const Button& p_button);
		void editWheelPosition(const Vector2Int& p_delta);

		void update();

		const InputState& getButton(const Button& p_button) const;

		const Vector2Int& position() const;
		const Vector2Int& deltaPosition() const;
		const Vector2Int& wheel() const;
	};
}