#include "input/spk_mouse.hpp"

namespace spk
{
    Mouse::Mouse() :
        _position(0),
        _deltaPosition(0),
        _wheel(0)
    {
        for (size_t i = 0; i < NB_BUTTON; i++)
        {
            _buttons[i] = InputState::Up;
        }
    }

    Mouse::~Mouse()
    {
    }

    void Mouse::setMousePosition(const Vector2Int& p_newPosition)
    {
        if (_deltaPosition == INITIAL_DELTA_POSITION)
            _deltaPosition = 0;
        else
            _deltaPosition = p_newPosition - _position;
        _position = p_newPosition;
    }

    void Mouse::pressButton(const Button& p_button)
    {
        if (_buttons[static_cast<size_t>(p_button)] != InputState::Down)
            _buttons[static_cast<size_t>(p_button)] = InputState::Pressed;
    }

    void Mouse::releaseButton(const Button& p_button)
    {
        if (_buttons[static_cast<size_t>(p_button)] != InputState::Up)
            _buttons[static_cast<size_t>(p_button)] = InputState::Released;
    }

    void Mouse::editWheelPosition(const Vector2Int& p_delta)
    {
        _wheel += p_delta;
    }

    void Mouse::update()
    {
        _deltaPosition.x = 0;
        _deltaPosition.y = 0;

        for (size_t i = 0; i < NB_BUTTON; i++)
        {
            if (_buttons[i] == InputState::Pressed)
            {
                _buttons[i] = InputState::Down;
            }
            else if (_buttons[i] == InputState::Released)
            {
                _buttons[i] = InputState::Up;
            }
        }
    }

    const InputState& Mouse::getButton(const Button& p_button) const
    {
        return (_buttons[static_cast<size_t>(p_button)]);
    }

    const Vector2Int& Mouse::position() const
    {
        return (_position);
    }

    const Vector2Int& Mouse::deltaPosition() const
    {
        return (_deltaPosition);
    }

    const Vector2Int& Mouse::wheel() const
    {
        return (_wheel);
    }
}