#pragma once

#include <iostream>
#include "math/spk_vector2.hpp"
#include "spk_basic_functions.hpp"
#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_text_label.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
    /**
     * @class TextLabel
     * @brief A widget class for displaying text within a UI, with a customizable background.
     *
     * This class extends Widget and includes a TextLabel for text rendering and a NineSlicedBox
     * for the background. It is designed to handle both the graphical representation and interaction
     * logic for text elements in the UI, supporting dynamic resizing and styling changes.
     *
     * TextLabel can be used to display any text information such as labels, button texts, or info panels.
     * The NineSlicedBox allows the background to dynamically resize with the text, maintaining padding
     * and aspect ratio consistency.
     *
     * Usage example:
     * @code
     * spk::TextLabel* myTextLabel = new spk::TextLabel(parentWidget);
     * myTextLabel->label().setFont(myFont);
     * myTextLabel->label().setText("Hello, world!");
     * myTextLabel->label().setFontSize(14);
     * myTextLabel->box().setSpriteSheet(myLabelBackground);
     * @endcode
     *
     * @note The class manages layout changes by overriding the `_onGeometryChange` and `_onRender` methods
     *       to ensure the text and its background are correctly sized and rendered.
     */
    class TextLabel : public Widget
    {
    private:
        spk::WidgetComponent::NineSlicedBox _box; ///< Background box that can be styled and resized.
        spk::WidgetComponent::TextLabel _label;   ///< Text component used for displaying the label's content.

        /**
         * Handles geometry changes, updating the layout of the text and background to fit the new widget size.
         */
        void _onGeometryChange() override;

        /**
         * Renders the text and its background to the screen.
         */
        void _onRender() override;

    public:
		/**
		 * @brief Construct a new TextLabel widget with a given parent.
         * @param p_parent Pointer to the parent widget.
		*/
        TextLabel(Widget* p_parent);
		
		/**
		 * @brief Construct a new TextLabel widget with a specific name and a given parent.
		 * @param p_name The desired name of the widget.
         * @param p_parent Pointer to the parent widget.
		*/
        TextLabel(const std::string& p_name, Widget* p_parent);

        /**
         * Provides non-const access to the internal TextLabel component.
         * @return Reference to the non-const TextLabel used by this widget for displaying text.
         */
        spk::WidgetComponent::TextLabel& label();

        /**
         * Provides non-const access to the internal NineSlicedBox component.
         * @return Reference to the non-const NineSlicedBox used by this widget for the background.
         */
        spk::WidgetComponent::NineSlicedBox& box();
    };
}
