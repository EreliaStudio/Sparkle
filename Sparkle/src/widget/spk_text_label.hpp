#pragma once

#include <iostream>

#include "math/spk_vector2.hpp"
#include "spk_basic_functions.hpp"
#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_text_label.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
    class TextLabel : public Widget
    {
    private:
        spk::WidgetComponent::NineSlicedBox _box;
        spk::WidgetComponent::TextLabel _label;
        Font* _font;
        std::string _text;

        void _onGeometryChange() override;

        void _onRender() override;

    public:
        TextLabel(Widget* p_parent = nullptr);

        spk::WidgetComponent::TextLabel& label();

        spk::WidgetComponent::NineSlicedBox& box();

        spk::Vector2Int calculateTextArea() const;
    };
}