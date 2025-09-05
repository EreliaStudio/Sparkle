#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/system/spk_input_state.hpp"

namespace spk
{
	class Window;
	class MouseModule;

	class Mouse
	{
		friend class MouseModule;

	public:
		enum class Button
		{
			Left = 0,
			Middle = 1,
			Right = 2,
			Unknow
		};

	private:
		Vector2Int _position;
		Vector2Int _deltaPosition;
		InputState _buttons[3];
		float _wheel;
		spk::SafePointer<Window> _window;

	public:
		Mouse();
		virtual ~Mouse() = default;
		InputState operator[](Button p_button) const;

		const Vector2Int &position() const;
		const Vector2Int &deltaPosition() const;
		const InputState *buttons() const;
		float wheel() const;
		spk::SafePointer<Window> window() const;

		void place(const spk::Vector2Int &p_newPosition) const;
	};
}

std::ostream &operator<<(std::ostream &p_os, const spk::Mouse::Button &p_button);
std::wostream &operator<<(std::wostream &p_os, const spk::Mouse::Button &p_button);
