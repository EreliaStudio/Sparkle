#pragma once

#include "graphics/spk_color.hpp"
#include "math/spk_vector2.hpp"
#include "widget/components/spk_colored_box.hpp"
#include "widget/spk_box_constraints.hpp"
#include "widget/spk_single_child_widget.hpp"
#include "widget/spk_widget.hpp"
#include <functional>

/**
 * @file
 *
 * This file contains a collection of basic widgets that mostly serve to build basic layouts.
 */

namespace spk::widget
{
    /**
     * @class ColoredBox
     * @brief A box filled with color.
     */
    class ColoredBox : public SingleChildWidget
    {
    private:
        spk::widget::components::ColoredBox _coloredBox;

    protected:
        void _onGeometryChange() override;
        void _onRender() override;

    public:
        /**
         * @brief Contructor with a name parameter.
         * @param p_name The name of the widget, used for debug.
         * @param p_color The color used to fill the box.
         * @param p_parent The parent widget.
         */
        ColoredBox(const std::string& p_name, const spk::Color& p_color, IWidget* p_parent);

        /**
         * @brief Contructor with a default name.
         * @param p_color The color used to fill the box.
         * @param p_parent The parent widget.
         */
        ColoredBox(const spk::Color& p_color, IWidget* p_parent = nullptr);

        virtual ~ColoredBox() = default;
    };

    /**
     * @class LayoutBuilder
     * @brief LayoutBuilder allows defining layout through a Builder without writing a new class.
     */
    class LayoutBuilder : public SingleChildWidget
    {
    public:
        /**
         * @brief Builder is used to inject some logic for the layout of LayoutBuilder's child.
         */
        using Builder = std::function<Vector2(const BoxConstraints&, IWidget* child)>;

        /**
         * @brief Constructor.
         * @param p_builder The builder callback.
         * @param p_parent The parent widget.
         */
        LayoutBuilder(const Builder& p_builder, IWidget* p_parent = nullptr);

        /**
         * @brief Implement layout to set the child's size according to the builder method.
         */
        Vector2 layout(const BoxConstraints& p_constraints);

    private:
        Builder _builder;
    };

    /**
     * @brief SizedBox forces its child to a fixed size.
     */
    class SizedBox : public SingleChildWidget
    {
    public:
        /**
         * @brief Constructor.
         * @param p_size The desired size of the widget.
         * @param p_parent The parent widget.
         */
        SizedBox(const Vector2& p_size, IWidget* p_parent = nullptr);

        /**
         * @brief Implement layout to set the child's size to the desired value.
         */
        Vector2 layout(const BoxConstraints& p_constraints) override;

    private:
        Vector2 _size;
    };

    /**
     * @brief FractionallySizedBox sizes its widgets to a fraction of its parent's constraints.
     */
    class FractionallySizedBox : public SingleChildWidget
    {
    public:
        /**
         * @brief Constructor.
         * @param p_horizontal The horizontal fractional value. Set to 1 to get the same width as the parent.
         * @param p_vertical The vertical fractional value. Set to 1 to get the same height as the parent.
         * @param p_parent The parent widget.
         */
        FractionallySizedBox(float p_horizontal, float p_vertical, IWidget* p_parent = nullptr);

        /**
         * @brief Implement layout to set the child's size to a fraction of the parent's size.
         */
        Vector2 layout(const BoxConstraints& p_constraints) override;

    private:
        float _horizontal;
        float _vertical;
    };

    /**
     * @brief Padding adds padding around its child.
     */
    class Padding : public SingleChildWidget
    {
    public:
        /**
         * @brief A padding configuration.
         */
        struct Config
        {
            float left;   //!< left padding in pixels
            float right;  //!< right padding in pixels
            float top;    //!< top padding in pixels
            float bottom; //!< bottom padding in pixels

            /**
             * @brief Default constructor to set all sides to 0.
             */
            Config();

            /**
             * @brief Constructor.
             *
             * @param p_left left padding.
             * @param p_right right padding.
             * @param p_top top padding.
             * @param p_bottom bottom padding.
             */
            Config(float p_left, float p_right, float p_top, float p_bottom);

            /**
             * @brief Constructor with all sided equal.
             *
             */
            static Config all(float p_pad);
        };

        /**
         * @brief Constructor.
         * @param p_config The padding configuration.
         * @param p_parent The parent widget.
         */
        Padding(const Config& p_config, IWidget* p_parent = nullptr);

        /**
         * @brief implement layout to add padding to the child.
         */
        Vector2 layout(const BoxConstraints& p_constraints) override;

    private:
        Config _config;
    };

    /**
     * @brief Expanded allows flexible sizing of its child.
     *
     * It must be used with a Column or a Row.
     * The child will share the available space with other Expanded widgets.
     */
    class Expanded : public SingleChildWidget
    {
    public:
        /**
         * @brief Constructor.
         * @param p_flex Set this value to give different portions of the available space to each Expanded widget. Default to 1.0f
         * @param p_parent The parent widget.
         */
        Expanded(float p_flex, IWidget* p_parent = nullptr);

        /**
         * Constructor with default flex of 1.0f.
         * @param p_parent The parent widget.
         */
        Expanded(IWidget* p_parent = nullptr);

        /**
         * @brief Implement layout to always use the full size available.
         */
        Vector2 layout(const BoxConstraints& p_constraints) override;

        /**
         * @brief Getter for flex.
         */
        float flex() const;

    private:
        float _flex;
    };

    /**
     * @brief MainAxisAlignment describes the alignment for the main axis.
     * i.e. the vertical axis for Column, the horizontal axis for Row.
     */
    enum class MainAxisAlignment
    {
        start,
        center,
        end
    };

    /**
     * @brief MainAxisAlignment describes the alignment for the cross axis.
     * i.e. the horizontal axis for Column, the vertical axis for Row.
     */
    enum class CrossAxisAlignment
    {
        start,
        center,
        end
    };

    /**
     * @brief Column allows laying out multiple widgets vertically.
     *
     * The children are either layed out used their intrisic size, or can be wrapped in an Expanded
     * widget to adapt to the available space.
     */
    class Column : public IWidget
    {
    public:
        /**
         * @brief Configuration for Column.
         */
        struct Config
        {
            MainAxisAlignment mainAxisAlignment;   //!< main axis alignment
            CrossAxisAlignment crossAxisAlignment; //!< cross axis alignment

            /**
             * @brief Default constructor.
             *
             * Sets the MainAxisAlignment to start.
             * Sets the CrossAxisAlignement to start.
             */
            Config();
        };

        /**
         * @brief Constructor.
         * @param p_config The config for this column.
         * @param p_parent The parent widget.
         */
        Column(const Config& p_config, IWidget* p_parent = nullptr);

        /**
         * @brief Constructor with default configuration.
         * @param p_parent The parent widget.
         */
        Column(IWidget* p_parent = nullptr);

        /**
         * @brief Implement layout to align the children vertically.
         */
        Vector2 layout(const BoxConstraints& p_constraints) override;

    private:
        Config _config;
    };

    /**
     * @brief Row allows laying out multiple widgets horizontally.
     *
     * The children are either layed out used their intrisic size, or can be wrapped in an Expanded
     * widget to adapt to the available space.
     */
    class Row : public IWidget
    {
    public:
        /**
         * @brief Configuration for Row.
         */
        struct Config
        {
            MainAxisAlignment mainAxisAlignment;   //!< main axis alignment.
            CrossAxisAlignment crossAxisAlignment; //!< cross axis alignment.

            /**
             * @brief Default constructor.
             *
             * Sets the MainAxisAlignment to start.
             * Sets the CrossAxisAlignement to start.
             */
            Config();
        };

        /**
         * @brief Constructor.
         * @param p_config The row configuration.
         * @param p_parent The parent widget.
         */
        Row(const Config& p_config, IWidget* p_parent = nullptr);

        /**
         * @brief Constructor with default configuration.
         * @param p_parent The parent widget.
         */
        Row(IWidget* p_parent = nullptr);

        /**
         * @brief Implement layout to align the children horizontally.
         */
        Vector2 layout(const BoxConstraints& p_constraints) override;

    private:
        Config _config;
    };

    /**
     * @brief Centers the child.
     */
    class Center : public SingleChildWidget
    {
    public:
        /**
         * @brief Constructor.
         * @param p_parent The parent widget.
         */
        Center(IWidget* p_parent = nullptr);

        /**
         * @brief Implement layout to center the child.
         */
        Vector2 layout(const BoxConstraints& p_constraints);
    };
}
