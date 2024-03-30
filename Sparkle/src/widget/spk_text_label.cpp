#include "widget/spk_text_label.hpp"

namespace spk::widget
{
    void TextLabel::_onGeometryChange()
    {
        _box.setGeometry(anchor(), size());
        _box.setLayer(layer());

        _label.setAnchor(anchor() + size() / 2);
        _label.setLayer(layer() + 0.01f);
    }

    void TextLabel::_onRender()
    {
        _box.render();
        _label.render();
    }

    TextLabel::TextLabel(IWidget* p_parent) :
        IWidget(p_parent),
        _font(nullptr)
    {

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

    spk::widget::components::NineSlicedBox& TextLabel::box()
    {
        return (_box);
    }

    spk::Vector2Int TextLabel::calculateTextArea() const
    {
        return (_label.calculateTextArea());
    }
}