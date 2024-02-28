#pragma once

#include "widget/spk_single_child_widget.hpp"
#include <functional>

namespace spk::widget
{
    /**
     * @brief Detects if a pointer enters this widget's children.
     */
    class PointerDetector : public SingleChildWidget
    {
    public:
        /**
         * @brief Constructor.
         * @param p_parent The parent widget.
         */
        PointerDetector(IWidget* p_parent = nullptr);

        /// The type of function called on an event.
        using Callback = std::function<void()>;

        /**
         * @brief Sets the callback to call when the widget is pressed.
         * @param p_onPressed The callback to call.
         */
        void setOnPressed(const Callback& p_onPressed);

        /**
         * @brief Sets the callback to call when the widget is released.
         * @param p_onReleased The callback to call.
         */
        void setOnReleased(const Callback& p_onReleased);

        /**
         * @brief Sets the callback to call when the widget is enter.
         * @param p_onEnter The callback to call.
         */
        void setOnEnter(const Callback& p_onEnter);

        /**
         * @brief Sets the callback to call when the widget is exit.
         * @param p_onExit The callback to call.
         */
        void setOnExit(const Callback& p_onExit);

        /// @brief Getter for isPressed.
        /// @return true if the widget is currently pressed.
        bool isPressed() const;

        /// @brief Getter for isHovered.
        /// @return true if the widget is currently hovered.
        bool isHovered() const;

    private:
        void _onUpdate() override;
        bool leftMouseDown() const;

        Callback _onPressed;
        Callback _onReleased;
        Callback _onEnter;
        Callback _onExit;

        bool _isHovered = false;
        bool _isPressed = false;
    };
}