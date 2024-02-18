#pragma once

#include <iostream>

#include "math/spk_vector2.hpp"
#include "spk_basic_functions.hpp"
#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_text_label.hpp"
#include "widget/spk_widget.hpp"

namespace spk::widget
{
    class TextLabel : public IWidget
    {
    private:
        spk::widget::components::NineSlicedBox _box;
        spk::widget::components::TextLabel _label;
        Font* _font;
        std::string _text;

        void _onGeometryChange() override
        {
            IWidget::_onGeometryChange();
            _box.setGeometry(anchor(), size());
            _box.setDepth(depth());

            _label.setAnchor(anchor() + (size() / 2));
            _label.setDepth(depth() + 1);
        }

        void _onRender() override
        {
            DEBUG_LINE();
            //_box.render();
            _label.render();
        }

    public:
        TextLabel(const std::string& p_text, Font* p_font, IWidget* p_parent = nullptr) :
            IWidget("TextLabel", p_parent),
            _font(p_font),
            _text(p_text)
        {

            label().setFont(_font); // Assuming myFont is a preloaded Font instance
            label().setText(_text);
            label().setTextSize(50);
            label().setTextColor(spk::Color{255, 255, 255});
            label().setAnchor({10, 10});
            label().setVerticalAlignment(spk::VerticalAlignment::Centered);
            label().setHorizontalAlignment(spk::HorizontalAlignment::Left);
        }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override
        {
            // TODO: need to return textlabel layout inside those constraints;
            std::cout << "MAX " << p_constraints.max << std::endl;
            return p_constraints.max;
        }

        spk::widget::components::NineSlicedBox& box() { return (_box); }
        spk::widget::components::TextLabel& label() { return (_label); }
    };
}