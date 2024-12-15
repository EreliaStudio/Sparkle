#pragma once

#include "widget/spk_widget.hpp"

#include "widget/component/spk_color_box.hpp"
#include "widget/component/spk_nine_slice_renderer.hpp"
#include "widget/component/spk_text_renderer.hpp"

#include <array>
#include <functional>

namespace spk
{
    class Button : public spk::Widget
    {
    public:
        enum class State
        {
            Pressed,
            Released
        };

    private:
        void _onGeometryChange() override;
        void _onPaintEvent(spk::PaintEvent& p_event) override;
        void _onMouseEvent(spk::MouseEvent& p_event) override;

        void _updateRenderers();

        std::array<WidgetComponent::ColorBox, 2> _colorBoxes;
        std::array<WidgetComponent::NineSliceRenderer, 2> _nineSliceRenderers;
        std::array<WidgetComponent::TextRenderer, 2> _textRenderers;

        bool _isPressed = false;
        std::function<void()> _callback;

    public:
        Button(const std::wstring& p_name, const spk::SafePointer<Widget>& p_parent);
        ~Button();

        WidgetComponent::ColorBox& colorBox(State state);
        const WidgetComponent::ColorBox& colorBox(State state) const;

        WidgetComponent::NineSliceRenderer& nineSliceRenderer(State state);
        const WidgetComponent::NineSliceRenderer& nineSliceRenderer(State state) const;

        WidgetComponent::TextRenderer& textRenderer(State state);
        const WidgetComponent::TextRenderer& textRenderer(State state) const;

        void setCallback(const std::function<void()>& callback);
        const std::function<void()>& callback() const;
    };
}
