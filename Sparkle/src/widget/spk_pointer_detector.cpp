#include "widget/spk_pointer_detector.hpp"
#include "application/spk_application.hpp"

namespace spk::widget
{

    PointerDetector::PointerDetector(IWidget* p_parent) :
        SingleChildWidget("PointerDetector", p_parent)
    {
    }

    void PointerDetector::setOnPressed(const Callback& p_onPressed)
    {
        _onPressed = p_onPressed;
    }

    void PointerDetector::setOnReleased(const Callback& p_onReleased)
    {
        _onReleased = p_onReleased;
    }

    void PointerDetector::setOnEnter(const Callback& p_onEnter)
    {
        _onEnter = p_onEnter;
    }

    void PointerDetector::setOnExit(const Callback& p_onExit)
    {
        _onExit = p_onExit;
    }

    bool PointerDetector::isPressed() const
    {
        return _isPressed;
    }

    bool PointerDetector::isHovered() const
    {
        return _isHovered;
    }

    void PointerDetector::_onUpdate()
    {
        IWidget* tmpChild = child();
        if (nullptr == tmpChild)
        {
            return;
        }

        Vector2Int mousePosition = spk::Application::activeApplication()->mouse().position();
        bool newHovered = tmpChild->hitTest(mousePosition);

        if (newHovered && !_isHovered)
        {
            if (nullptr != _onEnter)
            {
                _onEnter();
            }
        }
        else if (!newHovered && _isHovered)
        {
            if (nullptr != _onExit)
            {
                _onExit();
            }
        }

        // Update _hovered before checking for click state.
        _isHovered = newHovered;

        spk::InputState leftMouseState = spk::Application::activeApplication()->mouse().getButton(spk::Mouse::Left);

        if (_isHovered && leftMouseState == InputState::Pressed)
        {
            _isPressed = true;
            if (nullptr != _onPressed)
            {
                _onPressed();
            }
        }

        if (_isPressed && leftMouseState == InputState::Released)
        {
            _isPressed = false;
            if (nullptr != _onReleased)
            {
                _onReleased();
            }
        }
    }

    bool PointerDetector::leftMouseDown() const
    {
        spk::InputState leftMouseState = spk::Application::activeApplication()->mouse().getButton(spk::Mouse::Left);
        return leftMouseState == spk::InputState::Pressed;
    }
}