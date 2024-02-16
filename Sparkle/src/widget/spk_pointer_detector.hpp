#include "widget/spk_widget.hpp"

namespace spk::widget
{
    class PointerDetector : public IWidget
    {
    public:
        PointerDetector() = default;

        using Callback = std::function<void()>;
        void setOnPressed(const Callback &p_onPressed);
        void setOnReleased(const Callback &p_onReleased);
        void setOnEnter(const Callback &p_onEnter);
        void setOnExit(const Callback &p_onExit);

    private:
        void _onGeometryChange() override;
        void _onUpdate() override;
        void _onRender() override;
    };
}