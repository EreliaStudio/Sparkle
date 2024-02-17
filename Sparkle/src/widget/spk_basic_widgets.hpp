#pragma once

#include "widget/spk_widget.hpp"

namespace spk::widget
{

    /**
     * @brief LayoutBuilder allows defining _onLayout through a Builder without writing a new class.
     */
    class LayoutBuilder : public IWidget
    {
    public:
        using Builder = std::function<Vector2(const BoxConstraints&)>;
        LayoutBuilder(const Builder& p_builder, IWidget* p_parent) :
            IWidget("LayoutBuilder", p_parent),
            _builder(p_builder)
        {
        }

    private:
        Vector2 _onLayout(const BoxConstraints& p_constraints)
        {
            return _builder(p_constraints);
        }

        Builder _builder;
    };

    /**
     * @brief SizedBox forces its child to a fixed size.
     */
    class SizedBox : public SingleChildWidget
    {
    public:
        SizedBox(const Vector2& p_size, IWidget* p_parent) :
            SingleChildWidget("SizedBox", p_parent),
            _size(p_size)
        {
        }

        const Vector2& size() const
        {
            return _size;
        }

    private:
        Vector2 _onLayout(const BoxConstraints& p_constraints) override
        {
            IWidget* child = SingleChildWidget::child();
            if (child == nullptr)
            {
                return _size;
            }
            // Size that satisfies the constraints and the SizedBox params.
            Vector2 max = Vector2::min(p_constraints.max, _size);
            Vector2 min = Vector2::max(p_constraints.min, _size);

            Vector2 childSize = child->_onLayout(BoxConstraints(min, max));
            child->setGeometry({0, 0}, childSize);

            return max;
        }

        Vector2 _size;
    };

    /**
     * @brief FractionallySizedBox sizes its widgets to a fraction of its parent's constraints.
     */
    class FractionallySizedBox : public SingleChildWidget
    {
    public:
        FractionallySizedBox(float p_horizontal, float p_vertical, IWidget* p_parent) :
            SingleChildWidget("FractionallySizedBox", p_parent),
            _horizontal(p_horizontal),
            _vertical(p_vertical)
        {
        }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override
        {
            float w = p_constraints.max.x * _horizontal;
            if (w < p_constraints.min.x)
            {
                w = p_constraints.min.x;
            }

            float h = p_constraints.max.y * _vertical;
            if (h < p_constraints.min.y)
            {
                h = p_constraints.min.y;
            }

            Vector2 size{w, h};

            IWidget* child = SingleChildWidget::child();
            if (nullptr != child)
            {
                BoxConstraints constraints{p_constraints.min, size};
                Vector2 childSize = child->_onLayout(constraints);
                child->setGeometry({0, 0}, childSize);
            }

            return size;
        }

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
        struct Config
        {
            float left;
            float right;
            float top;
            float bottom;
        };

        Padding(const Config& p_config, IWidget* p_parent) :
            SingleChildWidget("Padding", p_parent),
            _config(p_config)
        {
        }

        static Padding symmetric(float p_horizontal, float p_vertical, IWidget* p_parent) { return Padding({p_horizontal, p_horizontal, p_vertical, p_vertical}, p_parent); }
        static Padding all(float p_value, IWidget* p_parent) { return Padding({p_value, p_value, p_value, p_value}, p_parent); }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override
        {
            IWidget* child = SingleChildWidget::child();
            if (nullptr != child)
            {
                Vector2 padded = Vector2{_config.left + _config.right, _config.top + _config.bottom};
                Vector2 max = p_constraints.max - padded;
                Vector2 min = p_constraints.min - padded;
                Vector2 childSize = child->_onLayout({min, max});

                Vector2 anchor{_config.left, _config.top};
                child->setGeometry(anchor, childSize);
            }

            return p_constraints.max;
        }

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
         */
        Expanded(float p_flex, IWidget* p_parent) :
            SingleChildWidget("Expanded", p_parent),
            _flex(p_flex)
        {
        }
        Expanded(IWidget* p_parent) :
            Expanded(1.0f, p_parent)
        {
        }

        float flex() const { return _flex; }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override
        {
            IWidget* child = SingleChildWidget::child();
            if (child != nullptr)
            {
                return p_constraints.min;
            }
            Vector2 childSize = child->_onLayout(p_constraints);
            child->setGeometry({0, 0}, childSize);
            return childSize;
        }

    private:
        float _flex;
    };

    enum class MainAxisAlignment
    {
        start,
        center,
        end
    };

    enum class CrossAxisAlignment
    {
        start,
        center,
        end
    };

    class Column : public IWidget
    {
    public:
        struct Config
        {
            MainAxisAlignment mainAxisAlignment;
            CrossAxisAlignment crossAxisAlignment;

            Config() :
                mainAxisAlignment(MainAxisAlignment::start),
                crossAxisAlignment(CrossAxisAlignment::start)
            {
            }
        };

        Column(const Config& p_config, IWidget* p_parent) :
            IWidget("Column", p_parent),
            _config(p_config)
        {
        }

        Column(IWidget* p_parent) :
            Column(Config(), p_parent)
        {
        }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override
        {
            size_t len = children().size();
            std::vector<Vector2> sizes(len, {0, 0});

            float totalFlex = 0;
            float height = 0;
            // Start by getting the size of children that have an intrisic size.
            for (size_t i = 0; i < len; i++)
            {
                IWidget* child = children()[i];
                Expanded* asExpanded = dynamic_cast<Expanded*>(child);
                if (nullptr == asExpanded)
                {
                    Vector2 childSize = child->_onLayout(p_constraints);
                    sizes[i] = childSize;

                    // Keep track of the total height from these widgets.
                    height += childSize.y;
                }
                else
                {
                    // Take advantage of the loop to count the total flex during this pass.
                    totalFlex += asExpanded->flex();
                }
            }

            // Find the height available for our flexible widgets.
            float availableHeight = p_constraints.max.y - height;
            if (availableHeight < 0)
            {
                availableHeight = 0;
            }

            // Now get the sizes of the flexible children.
            for (size_t i = 0; i < len; i++)
            {
                IWidget* child = children()[i];
                Expanded* asExpanded = dynamic_cast<Expanded*>(child);
                if (nullptr != asExpanded)
                {
                    float flexFactor = totalFlex / asExpanded->flex();
                    float childHeight = availableHeight * flexFactor;
                    Vector2 max{p_constraints.max.x, childHeight};
                    Vector2 min{0, childHeight};

                    BoxConstraints constraints{min, max};
                    Vector2 childSize = asExpanded->_onLayout(constraints);

                    sizes[i] = childSize;
                    height += childHeight;
                }
            }

            // Finally we can compute positions for each widget.
            // Start by finding out at which position on y we should start.
            float baseY = 0.0;
            switch (_config.mainAxisAlignment)
            {
            case MainAxisAlignment::start:
                baseY = 0.0;
                break;
            case MainAxisAlignment::center:
                baseY = (p_constraints.max.y - height) / 2;
                break;
            case MainAxisAlignment::end:
                baseY = p_constraints.max.y - height;
                break;
            }

            float currentY = baseY;
            for (size_t i = 0; i < children().size(); i++)
            {
                const Vector2& size = sizes[i];
                IWidget* child = children()[i];

                // For each widget we must find the x coordinate relative to the CrossAxisAlignment.
                float x;
                switch (_config.crossAxisAlignment)
                {
                case CrossAxisAlignment::start:
                    x = 0.0;
                    break;
                case CrossAxisAlignment::center:
                    x = (p_constraints.max.x - size.x) / 2;
                    break;
                case CrossAxisAlignment::end:
                    x = p_constraints.max.x - size.x;
                    break;
                }

                Vector2 anchor{x, currentY};
                child->setGeometry(anchor, size);

                // Make sure to update the currentY for the next widget.
                currentY += size.y;
            }

            return p_constraints.max;
        }

    private:
        Config _config;
    };

    class Row : public IWidget
    {
    public:
        struct Config
        {
            MainAxisAlignment mainAxisAlignment;
            CrossAxisAlignment crossAxisAlignment;

            Config() :
                mainAxisAlignment(MainAxisAlignment::start),
                crossAxisAlignment(CrossAxisAlignment::start)
            {
            }
        };

        Row(const Config& p_config, IWidget* p_parent) :
            IWidget("Row", p_parent),
            _config(p_config)
        {
        }

        Row(IWidget* p_parent) :
            Row(Config(), p_parent)
        {
        }

        Vector2 _onLayout(const BoxConstraints& p_constraints) override
        {
            size_t len = children().size();
            std::vector<Vector2> sizes(len, {0, 0});

            float totalFlex = 0;
            float width = 0;
            // Start by getting the size of children that have an intrisic size.
            for (size_t i = 0; i < len; i++)
            {
                IWidget* child = children()[i];
                Expanded* asExpanded = dynamic_cast<Expanded*>(child);
                if (nullptr == asExpanded)
                {
                    Vector2 childSize = child->_onLayout(p_constraints);
                    sizes[i] = childSize;

                    // Keep track of the total width from these widgets.
                    width += childSize.x;
                }
                else
                {
                    // Take advantage of the loop to count the total flex during this pass.
                    totalFlex += asExpanded->flex();
                }
            }

            // Find the width available for our flexible widgets.
            float availableWidth = p_constraints.max.x - width;
            if (availableWidth < 0)
            {
                availableWidth = 0;
            }

            // Now get the sizes of the flexible children.
            for (size_t i = 0; i < len; i++)
            {
                IWidget* child = children()[i];
                Expanded* asExpanded = dynamic_cast<Expanded*>(child);
                if (nullptr != asExpanded)
                {
                    float flexFactor = totalFlex / asExpanded->flex();
                    float childWidth = availableWidth * flexFactor;
                    Vector2 max{childWidth, p_constraints.max.y};
                    Vector2 min{childWidth, 0};

                    BoxConstraints constraints{min, max};
                    Vector2 childSize = asExpanded->_onLayout(constraints);

                    sizes[i] = childSize;
                    width += childWidth;
                }
            }

            // Finally we can compute positions for each widget.
            // Start by finding out at which position on x we should start.
            float baseX = 0.0;
            switch (_config.mainAxisAlignment)
            {
            case MainAxisAlignment::start:
                baseX = 0.0;
                break;
            case MainAxisAlignment::center:
                baseX = (p_constraints.max.x - width) / 2;
                break;
            case MainAxisAlignment::end:
                baseX = p_constraints.max.x - width;
                break;
            }

            float currentX = baseX;
            for (size_t i = 0; i < children().size(); i++)
            {
                const Vector2& size = sizes[i];
                IWidget* child = children()[i];

                // For each widget we must find the y coordinate relative to the CrossAxisAlignment.
                float y;
                switch (_config.crossAxisAlignment)
                {
                case CrossAxisAlignment::start:
                    y = 0.0;
                    break;
                case CrossAxisAlignment::center:
                    y = (p_constraints.max.y - size.y) / 2;
                    break;
                case CrossAxisAlignment::end:
                    y = p_constraints.max.y - size.y;
                    break;
                }

                Vector2 anchor{currentX, y};
                child->setGeometry(anchor, size);

                // Make sure to update the currentX for the next widget.
                currentX += size.x;
            }

            return p_constraints.max;
        }

    private:
        Config _config;
    };

    class Center : public SingleChildWidget
    {
    public:
        Center(IWidget* p_parent) :
            SingleChildWidget("Center", p_parent)
        {
        }

        Vector2 _onLayout(const BoxConstraints& p_constraints)
        {
            IWidget* tmpChild = child();
            if (nullptr == tmpChild)
            {
                return p_constraints.min;
            }

            Vector2 childSize = tmpChild->_onLayout(p_constraints);
            Vector2 anchor = (p_constraints.max - childSize) / 2;
            tmpChild->setGeometry(anchor, childSize);
            return childSize;
        }
    };
}
