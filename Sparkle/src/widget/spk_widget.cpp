#include "widget/spk_widget.hpp"
#include "application/spk_application.hpp"

namespace spk::widget
{
    std::atomic_uint32_t IWidget::_nextId = 0;

    void IWidget::_onRender()
    {
    }

    void IWidget::_onUpdate()
    {
    }

    void IWidget::_onGeometryChange()
    {
        DLOG(name() << "#" << id() << ":" << anchor() << size() << "; depth=" << depth());
    }

    // By default the IWidget will layout its children by restricting their size to the constraints
    // he has been give, and aligning them to its top-left corner.
    Vector2 IWidget::layout(const BoxConstraints& p_constraints)
    {
        if (children().size() == 0)
        {
            return p_constraints.max;
        }
        // We aim at returning a box that englobes all children.
        Vector2 maxFromChildren{0, 0};
        for (auto& child : children())
        {
            // Get child size but cap it to match the constraints.
            Vector2 childSize = child->layout(p_constraints);
            child->setGeometry(anchor(), childSize);
            maxFromChildren = Vector2::max(maxFromChildren, childSize);
        }
        return maxFromChildren;
    }

    void IWidget::render()
    {
        _computeViewport();
        if (_viewport.size() <= Vector2{0, 0})
        {
            std::cout << "Viewport of " << _name << "#" << id() << " has a size of " << _viewport.size() << std::endl;
            return;
        }

        _onRender();
        for (auto& child : children())
        {
            if (child->isActive() == true)
            {
                _viewport.activate();
                child->render();
            }
        }
    }

    void IWidget::update()
    {
#ifndef NDEBUG
        _timeMetric.start();
#endif
        _onUpdate();
#ifndef NDEBUG
        _timeMetric.stop();
#endif

        for (auto& child : children())
        {
            if (child->isActive() == true)
            {
                child->update();
            }
        }
    }

    spk::Vector2Int IWidget::_computeAbsoluteAnchor()
    {
        spk::Vector2Int result = 0;
        const IWidget* tmp = this;

        while (tmp->parent() != nullptr)
        {
            result += tmp->anchor();
            tmp = tmp->parent();
        }

        return (result);
    }

    void IWidget::_computeViewport()
    {
        spk::Vector2Int topLeft = _computeAbsoluteAnchor();
        spk::Vector2Int rightDown = size() + topLeft;

        if (parent() != nullptr)
            topLeft = spk::Vector2Int::max(topLeft, parent()->viewport().anchor());

        if (parent() != nullptr)
            rightDown = spk::Vector2Int::min(rightDown, parent()->viewport().anchor() + parent()->viewport().size());

        _viewport.setGeometry(topLeft, rightDown - topLeft);
    }

    IWidget::IWidget(const std::string& p_name) :
        _id(_nextId++),
        _name(p_name),
#ifndef NDEBUG
        _timeMetric(spk::Application::activeApplication()->profiler().metric<TimeMetric>(name())),
#endif
        _depth(0)
    {
        if (defaultParent != nullptr)
            defaultParent->addChild(this);
    }

    IWidget::IWidget(const std::string& p_name, IWidget* p_parent) :
        IWidget(p_name)
    {
        if (p_parent != nullptr)
            p_parent->addChild(this);
    }

    IWidget::~IWidget()
    {
        if (parent() != nullptr)
        {
            transferChildrens(parent());
        }
    }

    void IWidget::addChild(IWidget* p_child)
    {
        TreeNode<IWidget>::addChild(p_child);
        p_child->setDepth(depth() + 1);
    }

    void IWidget::setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
    {
        if (p_anchor == _anchor && p_size == _size)
        {
            return;
        }
        _anchor = p_anchor;
        _size = p_size;

        _anchorRatio = (parent() != nullptr && parent()->size() != 0 ? static_cast<spk::Vector2>(_anchor) / static_cast<spk::Vector2>(parent()->size()) : 0);
        _sizeRatio = (parent() != nullptr && parent()->size() != 0 ? static_cast<spk::Vector2>(_size) / static_cast<spk::Vector2>(parent()->size()) : 1);
        _onGeometryChange();
    }

    void IWidget::setDepth(const float& p_depth)
    {
        _depth = p_depth;
    }

    void IWidget::activateAll()
    {
        activate();
        for (auto& child : children())
        {
            child->activateAll();
        }
    }

    void IWidget::deactivateAll()
    {
        deactivate();
        for (auto& child : children())
        {
            child->deactivateAll();
        }
    }

    bool IWidget::hitTest(const Vector2& p_coord)
    {
        Vector2 p0 = _viewport.anchor();
        Vector2 p1 = p0 + _viewport.size();
        return Vector2::isInsideRectangle(p_coord, p0, p1);
    }

    const std::string& IWidget::name() const
    {
        return (_name);
    }

    void IWidget::setName(const std::string& p_name)
    {
        _name = p_name;
    }

    const spk::Vector2Int& IWidget::anchor() const
    {
        return (_anchor);
    }

    const float& IWidget::depth() const
    {
        return (_depth);
    }

    const spk::Vector2UInt& IWidget::size() const
    {
        return (_size);
    }

    const spk::Viewport& IWidget::viewport() const
    {
        return (_viewport);
    }

    uint32_t IWidget::id() const
    {
        return _id;
    }

    SingleChildWidget::SingleChildWidget(const std::string& p_name, IWidget* p_parent) :
        IWidget(p_name, p_parent)
    {
    }

    IWidget* SingleChildWidget::child()
    {
        assert(children().size() < 2);

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
