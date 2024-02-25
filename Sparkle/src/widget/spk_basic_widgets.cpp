#include "widget/spk_basic_widgets.hpp"

namespace spk::widget
{

    ColoredBox::ColoredBox(const std::string& p_name, const spk::Color& p_color, IWidget* p_parent) :
        SingleChildWidget(p_name, p_parent)
    {
        _coloredBox.setColor(p_color);
    }

    ColoredBox::ColoredBox(const spk::Color& p_color, IWidget* p_parent) :
        ColoredBox("ColoredBox", p_color, p_parent)
    {
    }

    void ColoredBox::_onGeometryChange()
    {
        IWidget::_onGeometryChange();
        _coloredBox.setGeometry(anchor(), size());
        _coloredBox.setDepth(depth());
    }

    void ColoredBox::_onRender()
    {
        _coloredBox.render();
    }

    LayoutBuilder::LayoutBuilder(const Builder& p_builder, IWidget* p_parent) :
        SingleChildWidget("LayoutBuilder", p_parent),
        _builder(p_builder)
    {
    }

    Vector2 LayoutBuilder::layout(const BoxConstraints& p_constraints)
    {
        return _builder(p_constraints, child());
    }

    SizedBox::SizedBox(const Vector2& p_size, IWidget* p_parent) :
        SingleChildWidget("SizedBox", p_parent),
        _size(p_size)
    {
    }

    Vector2 SizedBox::layout(const BoxConstraints& p_constraints)
    {
        IWidget* tmpChild = child();
        if (tmpChild == nullptr)
        {
            return _size;
        }
        // Size that satisfies the constraints and the SizedBox params.
        Vector2 max = Vector2::min(p_constraints.max, _size);
        Vector2 min = Vector2::max(p_constraints.min, _size);

        Vector2 childSize = tmpChild->layout(BoxConstraints(min, max));
        tmpChild->setGeometry({0, 0}, childSize);

        return max;
    }

    const Vector2& SizedBox::size() const
    {
        return _size;
    }

    FractionallySizedBox::FractionallySizedBox(float p_horizontal, float p_vertical, IWidget* p_parent) :
        SingleChildWidget("FractionallySizedBox", p_parent),
        _horizontal(p_horizontal),
        _vertical(p_vertical)
    {
    }

    Vector2 FractionallySizedBox::layout(const BoxConstraints& p_constraints)
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
            Vector2 childSize = child->layout(constraints);
            child->setGeometry({0, 0}, childSize);
        }

        return size;
    }

    Padding::Config::Config() :
        left(0.0f),
        right(0.0f),
        top(0.0f),
        bottom(0.0f)
    {
    }

    Padding::Config::Config(float p_left, float p_right, float p_top, float p_bottom) :
        left(p_left),
        right(p_right),
        top(p_top),
        bottom(p_bottom)
    {
    }

    Padding::Config Padding::Config::all(float p_pad)
    {
        return Padding::Config(
            p_pad,
            p_pad,
            p_pad,
            p_pad);
    }

    Padding::Padding(const Config& p_config, IWidget* p_parent) :
        SingleChildWidget("Padding", p_parent),
        _config(p_config)
    {
    }

    Vector2 Padding::layout(const BoxConstraints& p_constraints)
    {
        IWidget* child = SingleChildWidget::child();
        if (nullptr != child)
        {
            Vector2 padded = Vector2{_config.left + _config.right, _config.top + _config.bottom};
            Vector2 max = p_constraints.max - padded;
            Vector2 min = p_constraints.min - padded;
            Vector2 childSize = child->layout({min, max});

            Vector2 anchor{_config.left, _config.top};
            child->setGeometry(anchor, childSize);
        }

        return p_constraints.max;
    }

    Expanded::Expanded(float p_flex, IWidget* p_parent) :
        SingleChildWidget("Expanded", p_parent),
        _flex(p_flex)
    {
    }

    Expanded::Expanded(IWidget* p_parent) :
        Expanded(1.0f, p_parent)
    {
    }

    Vector2 Expanded::layout(const BoxConstraints& p_constraints)
    {
        IWidget* tmpChild = child();
        if (nullptr == tmpChild)
        {
            return p_constraints.max;
        }
        Vector2 childSize = tmpChild->layout(p_constraints);
        Vector2 anchor = (p_constraints.max - childSize) / 2;
        tmpChild->setGeometry(anchor, childSize);
        return p_constraints.max;
    }

    float Expanded::flex() const
    {
        return _flex;
    }

    Column::Config::Config() :
        mainAxisAlignment(MainAxisAlignment::start),
        crossAxisAlignment(CrossAxisAlignment::start)
    {
    }

    Column::Column(const Config& p_config, IWidget* p_parent) :
        IWidget("Column", p_parent),
        _config(p_config)
    {
    }

    Column::Column(IWidget* p_parent) :
        Column(Config(), p_parent)
    {
    }

    Vector2 Column::layout(const BoxConstraints& p_constraints)
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
                Vector2 childSize = child->layout(p_constraints);
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
            // We don't like 0 height stuff.
            availableHeight = totalFlex + 1;
        }

        // Now get the sizes of the flexible children.
        for (size_t i = 0; i < len; i++)
        {
            IWidget* child = children()[i];
            Expanded* asExpanded = dynamic_cast<Expanded*>(child);
            if (nullptr != asExpanded)
            {
                float flexFactor = asExpanded->flex() / totalFlex;
                float childHeight = availableHeight * flexFactor;
                Vector2 max{p_constraints.max.x, childHeight};
                Vector2 min{0, childHeight};

                BoxConstraints constraints{min, max};
                Vector2 childSize = asExpanded->layout(constraints);

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

    Row::Config::Config() :
        mainAxisAlignment(MainAxisAlignment::start),
        crossAxisAlignment(CrossAxisAlignment::start)
    {
    }

    Row::Row(const Config& p_config, IWidget* p_parent) :
        IWidget("Row", p_parent),
        _config(p_config)
    {
    }

    Row::Row(IWidget* p_parent) :
        Row(Config(), p_parent)
    {
    }

    Vector2 Row::layout(const BoxConstraints& p_constraints)
    {
        size_t len = children().size();
        std::vector<Vector2> sizes(len, {1, 1});

        float totalFlex = 0;
        float width = 0;
        // Start by getting the size of children that have an intrisic size.
        for (size_t i = 0; i < len; i++)
        {
            IWidget* child = children()[i];
            Expanded* asExpanded = dynamic_cast<Expanded*>(child);
            if (nullptr == asExpanded)
            {
                Vector2 childSize = child->layout(p_constraints);
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
            availableWidth = totalFlex + 1;
        }

        // Now get the sizes of the flexible children.
        for (size_t i = 0; i < len; i++)
        {
            IWidget* child = children()[i];
            Expanded* asExpanded = dynamic_cast<Expanded*>(child);
            if (nullptr != asExpanded)
            {
                float flexFactor = asExpanded->flex() / totalFlex;
                float childWidth = availableWidth * flexFactor;
                Vector2 max{childWidth, p_constraints.max.y};
                Vector2 min{childWidth, 1};

                BoxConstraints constraints{min, max};
                Vector2 childSize = asExpanded->layout(constraints);

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

    Center::Center(IWidget* p_parent) :
        SingleChildWidget("Center", p_parent)
    {
    }

    Vector2 Center::layout(const BoxConstraints& p_constraints)
    {
        IWidget* tmpChild = child();
        if (nullptr == tmpChild)
        {
            return p_constraints.max;
        }

        Vector2 childSize = tmpChild->layout(p_constraints);
        Vector2 anchor = (p_constraints.max - childSize) / 2;
        tmpChild->setGeometry(anchor, childSize);
        return p_constraints.max;
    }
}