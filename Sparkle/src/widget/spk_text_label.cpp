#include "widget/spk_text_label.hpp"

namespace spk::widget
{
    void TextLabel::_onGeometryChange()
    {
        _label.setAnchor(anchor() + size() / 2);
        _label.setLayer(layer());
    }

    void TextLabel::_onRender()
    {
        _label.render();
    }

    TextLabel::TextLabel(IWidget* p_parent) :
        IWidget(p_parent),
        _font(nullptr),
        _text("")
    {
        label().setVerticalAlignment(spk::VerticalAlignment::Centered);
        label().setHorizontalAlignment(spk::HorizontalAlignment::Centered);
    }

    TextLabel::TextLabel(const std::string& p_text, Font* p_font, IWidget* p_parent) :
        TextLabel(p_parent)
    {
        _font = p_font;
        _text = p_text;
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