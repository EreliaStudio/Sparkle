#pragma once

#include "widget/spk_widget.hpp"

namespace spk::widget
{
    /**
     * @brief A widget that only has one child.
     *
     * This is mainly used to simplify the layout logics in widgets that inherit from this class.
     */
    class SingleChildWidget : public IWidget
    {
    public:
        /**
         * Constructor for creating a widget without a name and set the parent widget.
         * @param p_parent A pointer to the parent widget. This widget will be added as a child of the given parent.
         */
        SingleChildWidget(IWidget* p_parent = nullptr);
        
        /**
         * Constructor for creating a widget without a name and set the parent widget.
         * @param p_parent A pointer to the parent widget. This widget will be added as a child of the given parent.
         */
        SingleChildWidget(IWidget* p_parent = nullptr);
        
        /**
         * @brief Constructor
         *
         * @param p_name The debug name.
         * @param p_parent The parent widget.
         */
        SingleChildWidget(const std::string& p_name, IWidget* p_parent = nullptr);

        /**
         * @brief Getter for the single child of this widget.
         * @return A pointer to the single child, if any; nullptr if there is none.
         */
        IWidget* child();

        /**
         * @brief layout will pass down the constraints to the single child.
         * @param p_constraints The constraints for this layout.
         */
        Vector2 layout(const BoxConstraints& p_constraints) override;
    };

}