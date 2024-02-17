#pragma once

#include "widget/spk_widget.hpp"
#include <functional>

namespace spk::widget
{
    /**
     * @brief Detects if a pointer enters any of this widget's children.
     */
    class PointerDetector : public SingleChildWidget
    {
    public:
        PointerDetector(IWidget* p_parent) :
            SingleChildWidget("PointerDetector", p_parent)
        {
        }

        using Callback = std::function<void()>;

        void setOnPressed(const Callback& p_onPressed);
        void setOnReleased(const Callback& p_onReleased);
        void setOnEnter(const Callback& p_onEnter);
        void setOnExit(const Callback& p_onExit);

        bool isPressed() const;
        bool isHovered() const;

    private:
        void _onUpdate() override;
        bool leftMouseDown() const;

        Callback _onPressed;
        Callback _onReleased;
        Callback _onEnter;
        Callback _onExit;

        bool _hovered = false;
        bool _isClicked = false;
    };
}