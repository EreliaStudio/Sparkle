#include "widget/spk_widget.hpp"

namespace spk::widget
{
    /**
     * @brief Detects if a pointer enters any of this widget's children.
     */
    class PointerDetector : public IWidget
    {
    public:
        PointerDetector() = default;

        using Callback = std::function<void()>;

        void setOnPressed(const Callback& p_onPressed);
        void setOnReleased(const Callback& p_onReleased);
        void setOnEnter(const Callback& p_onEnter);
        void setOnExit(const Callback& p_onExit);

        bool isPressed() const;
        bool isHovered() const;

    private:
        void _onGeometryChange() override;
        void _onUpdate() override;
        void _onRender() override;

        Callback _onPressed;
        Callback _onReleased;
        Callback _onEnter;
        Callback _onExit;
    };
}