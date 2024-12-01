#include "widget/spk_button.hpp"

namespace spk
{
    Button::Button(const std::wstring& p_name, const spk::SafePointer<Widget>& p_parent)
        : spk::Widget(p_name, p_parent),
        _isPressed(false)
    {
        for (auto state : { State::Pressed, State::Released })
        {
            colorBox(state).setLayer(layer());
            colorBox(state).setColors(spk::Color(175, 175, 175), spk::Color(205, 205, 205));
            colorBox(state).setCornerSize({ 16, 16 });

            textRenderer(state).setLayer(layer() + 0.1f);
            textRenderer(state).setText(L"Button");
            textRenderer(state).setFontSize({ 24, 4 });
            textRenderer(state).setColors(spk::Color::black, spk::Color::white);

            nineSliceRenderer(state).setLayer(layer());
            nineSliceRenderer(state).setCornerSize({ 16, 16 });
        }

        colorBox(State::Pressed).setColors(spk::Color(125, 125, 125), spk::Color(155, 155, 155));
    }

    Button::~Button()
    {
    }

    WidgetComponent::ColorBox& Button::colorBox(State p_state)
    {
        return _colorBoxes[static_cast<size_t>(p_state)];
    }

    const WidgetComponent::ColorBox& Button::colorBox(State p_state) const
    {
        return _colorBoxes[static_cast<size_t>(p_state)];
    }

    WidgetComponent::NineSliceRenderer& Button::nineSliceRenderer(State p_state)
    {
        return _nineSliceRenderers[static_cast<size_t>(p_state)];
    }

    const WidgetComponent::NineSliceRenderer& Button::nineSliceRenderer(State p_state) const
    {
        return _nineSliceRenderers[static_cast<size_t>(p_state)];
    }

    WidgetComponent::TextRenderer& Button::textRenderer(State p_state)
    {
        return _textRenderers[static_cast<size_t>(p_state)];
    }

    const WidgetComponent::TextRenderer& Button::textRenderer(State p_state) const
    {
        return _textRenderers[static_cast<size_t>(p_state)];
    }

    void Button::setCallback(const std::function<void()>& callback)
    {
        _callback = callback;
    }

    const std::function<void()>& Button::callback() const
    {
        return _callback;
    }

    void Button::_onGeometryChange()
    {
        _updateRenderers();
    }

    void Button::_onPaintEvent(const spk::PaintEvent& p_event)
    {
        State currentState = _isPressed ? State::Pressed : State::Released;

        if (nineSliceRenderer(currentState).texture() != nullptr)
        {
            nineSliceRenderer(currentState).render();
        }
        else
        {
            colorBox(currentState).render();
        }

        textRenderer(currentState).render();
    }

    void Button::_onMouseEvent(const spk::MouseEvent& p_event)
    {
        switch (p_event.type)
        {
        case spk::MouseEvent::Type::Press:
            if (p_event.button == spk::Mouse::Button::Left)
            {
                if (viewport().geometry().contains(p_event.mouse->position))
                {
                    _isPressed = true;
                    p_event.requestPaint();
                    p_event.consume();
                }
            }
            break;
        case spk::MouseEvent::Type::Release:
            if (p_event.button == spk::Mouse::Button::Left)
            {
                if (_isPressed)
                {
                    if (viewport().geometry().contains(p_event.mouse->position))
                    {
                        if (_callback)
                        {
                            _callback();
                        }
                    }
                    _isPressed = false;
                    p_event.requestPaint();
                    p_event.consume();
                }
            }
            break;
        case spk::MouseEvent::Type::Motion:
            if (_isPressed)
            {
                if (viewport().geometry().contains(p_event.mouse->position) == false)
                {
                    p_event.requestPaint();
                    _isPressed = false;
                }
            }
            break;
        default:
            break;
        }
    }

    void Button::_updateRenderers()
    {
        for (auto state : { State::Pressed, State::Released })
        {
            colorBox(state).setGeometry(geometry());
            nineSliceRenderer(state).setGeometry(geometry());
            textRenderer(state).setGeometry(geometry());
        }
    }
}
