#include "widget/spk_text_label.hpp"

namespace spk
{
    void TextLabel::_onGeometryChange()
    {
        _box.setGeometry(anchor(), size());
        _box.setLayer(layer());

        _label.setGeometry(anchor() + _box.cornerSize(), size() - _box.cornerSize() * 2);
        _label.setLayer(layer() + 0.01f);
    }

    void TextLabel::_onRender()
    {
        _box.render();
        _label.render();
    }

    TextLabel::TextLabel(Widget* p_parent) :
        Widget(p_parent),
        _font(nullptr)
    {

    }

    spk::WidgetComponent::TextLabel& TextLabel::label()
    {
        return (_label);
    }

    spk::WidgetComponent::NineSlicedBox& TextLabel::box()
    {
        return (_box);
    }

    spk::Vector2Int TextLabel::calculateTextArea() const
    {
        return (_label.calculateTextArea());
    }
}