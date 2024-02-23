#include "widget/spk_text_label.hpp"

namespace spk::widget
{
    void TextLabel::_onGeometryChange()
    {
        IWidget::_onGeometryChange();

        _label.setAnchor(anchor());
        _label.setDepth(depth());
    }
    void TextLabel::_onRender()
    {
        _label.render();
    }

    TextLabel::TextLabel(const std::string& p_text, Font* p_font, IWidget* p_parent) :
        IWidget("TextLabel", p_parent),
        _font(p_font),
        _text(p_text)
    {

        label().setFont(_font); // Assuming myFont is a preloaded Font instance
        label().setText(_text);
        label().setTextSize(50);
        label().setTextColor(spk::Color{255, 255, 255});
        label().setAnchor(anchor());
        label().setVerticalAlignment(spk::VerticalAlignment::Top);
        label().setHorizontalAlignment(spk::HorizontalAlignment::Left);
    }

    Vector2 TextLabel::layout(const BoxConstraints& p_constraints)
    {
        // TODO: need to return textlabel layout inside those constraints;
        return p_constraints.max;
    }

    spk::widget::components::TextLabel& TextLabel::label()
    {
        return (_label);
    }
}