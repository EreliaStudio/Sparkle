#include "widget/spk_single_child_widget.hpp"
#include "spk_basic_functions.hpp"

namespace spk::widget
{
    SingleChildWidget::SingleChildWidget(const std::string& p_name, IWidget* p_parent) :
        IWidget(p_name, p_parent)
    {
    }

    SingleChildWidget::SingleChildWidget(IWidget* p_parent) :
        IWidget(p_parent)
    {

    }

    IWidget* SingleChildWidget::child()
    {
        if (children().size() > 1)
        {
            LOG_FATAL("SingleChildWidget " << name() << "#" << id() << " has more than one child");
        }

        if (children().size() < 1)
        {
            return nullptr;
        }
        return children()[0];
    }

    Vector2 SingleChildWidget::layout(const BoxConstraints& p_constraints)
    {
        IWidget* tmpChild = child();
        if (nullptr == tmpChild)
        {
            return p_constraints.max;
        }
        Vector2 childSize = tmpChild->layout(p_constraints);
        tmpChild->setGeometry({0, 0}, childSize);
        return childSize;
    }
}