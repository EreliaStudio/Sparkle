#pragma once

#include <iostream>

#include "math/spk_vector2.hpp"
#include "spk_basic_functions.hpp"
#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_text_label.hpp"
#include "widget/spk_widget.hpp"

namespace spk::widget
{
    /**
     * @brief A simple TextLabel :)
     */
    class TextLabel : public IWidget
    {
    private:
        spk::widget::components::NineSlicedBox _box;
        spk::widget::components::TextLabel _label;
        Font* _font;
        std::string _text;

        void _onGeometryChange() override;

        void _onRender() override;

    public:
        /**
         * @brief Default constructor
         * @param p_parent The parent widget.
        */
        TextLabel(IWidget* p_parent = nullptr);

        /**
         * @brief Implement layout to be able to tell the size of this label.
         */
        Vector2 layout(const BoxConstraints& p_constraints);

        /**
         * @brief Getter for the underlying label component.
         */
        spk::widget::components::TextLabel& label();

        spk::widget::components::NineSlicedBox& box();

        spk::Vector2Int calculateTextArea() const;
    };
}