#include "application/spk_application.hpp"
#include "widget/spk_widget.hpp"

namespace spk::widget
{
    // By default the IWidget will layout its children by restricting their size to the constraints
    // he has been give, and aligning them to its top-left corner.
    Vector2 IWidget::_onLayout(const BoxConstraints& p_constraints)
    {
        // We aim at returning a box that englobes all children.
        Vector2 maxFromChildren;
        for (auto& child : children())
        {
            // Get child size but cap it to match the constraints.
            Vector2 childSize = Vector2::min(p_constraints.max, child->layoutChildren(p_constraints));
            child->setGeometry({0, 0}, childSize);
            maxFromChildren = Vector2::max(maxFromChildren, childSize);
        }
        return maxFromChildren;
    }

    void IWidget::_onRender()
    {
    }

    void IWidget::_onUpdate()
    {
    }

    void IWidget::layout(const BoxConstraints& p_constraints)
    {
      _onLayout(p_contraints);
    }

    void IWidget::render()
    {
        _computeViewport();
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

    // void IWidget::resize(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
    // {
    // 	_anchor = p_anchor;
    // 	_size = p_size;
    // 	_needGeometryChange = true;
    //
    // 	for (auto& child : children())
    // 	{
    // 		child->resize(_size * child->_anchorRatio, _size * child->_sizeRatio);
    // 	}
    // 	_onGeometryChange();
    // }

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

    void IWidget::addChild(IWidget* p_children)
    {
        TreeNode<IWidget>::addChild(p_children);
        p_children->setDepth(depth() + 1);
    }

    void IWidget::setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
    {
        _anchor = p_anchor;
        _size = p_size;

        _anchorRatio = (parent() != nullptr && parent()->size() != 0 ? static_cast<spk::Vector2>(_anchor) / static_cast<spk::Vector2>(parent()->size()) : 0);
        _sizeRatio = (parent() != nullptr && parent()->size() != 0 ? static_cast<spk::Vector2>(_size) / static_cast<spk::Vector2>(parent()->size()) : 1);
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
        bool vIn = p_coord.x >= p0.x && p_coord.x <= p1.x;
        bool hIn = p_coord.y >= p0.y && p_coord.y <= p1.y;
        return vIn && hIn;
    }

    const std::string& IWidget::name() const
    {
        return (_name);
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
}
