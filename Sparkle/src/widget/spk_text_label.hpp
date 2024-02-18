#pragma once

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

        void _onGeometryChange() override
        {
            _box.setGeometry(anchor(), size());
            _box.setDepth(depth());

            _label.setAnchor(anchor() + (size() / 2));
            _label.setDepth(depth() + 1);
        }

        void _onRender() override
        {
            std::cout << "Can we render sometimes ?" << std::endl;
            DEBUG_LINE();
            _box.render();
            _label.render();
        }

    public:
        TextLabel(const std::string& p_name, IWidget* p_parent = nullptr) :
            IWidget(p_name, p_parent)
        {
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