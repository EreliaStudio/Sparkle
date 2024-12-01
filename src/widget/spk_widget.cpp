#include "widget/spk_widget.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{

	Widget::Widget(const std::wstring& p_name) :
		_name(p_name),
		_parent(nullptr),
		_layer(0)
	{

	}

	Widget::Widget(const std::wstring& p_name, spk::SafePointer<Widget> p_parent) :
		Widget(p_name)
	{
		if (p_parent != nullptr)
			p_parent->addChild(this);
	}

	Widget::~Widget()
	{
		if (_parent != nullptr)
		{	
			_parent->removeChild(this);
		}

		for (auto& child : _managedChildren)
		{
			delete child;
		}
	}

	const std::wstring& Widget::name() const
	{
		return (_name);
	}

	void Widget::addChild(spk::SafePointer<Widget> p_child)
	{
		spk::InherenceObject<Widget>::addChild(p_child);
		p_child->setLayer(layer() + 1);
	}

	void Widget::setLayer(const float& p_layer)
	{
		float delta = p_layer - _layer;
		for (auto& child : children())
		{
			static_cast<Widget*>(child)->setLayer(static_cast<Widget*>(child)->layer() + delta);
		}
		_layer = p_layer;
	}

	const float& Widget::layer() const
	{
		return (_layer);
	}
	
	void Widget::_computeRatio()
	{
		if (parent() == nullptr)
		{
			_anchorRatio = 1;
			_sizeRatio = 1;
		}
		else
		{
			_anchorRatio = spk::Vector2(geometry().anchor) / spk::Vector2(static_cast<Widget*>(parent())->geometry().size);
			_sizeRatio = spk::Vector2(geometry().size) / spk::Vector2(static_cast<Widget*>(parent())->geometry().size);
		}
	}

	void Widget::_resize()
	{
		_geometry.anchor = static_cast<Widget*>(parent())->geometry().size * _anchorRatio;
		_geometry.size = static_cast<Widget*>(parent())->geometry().size * _sizeRatio;
		requireGeometryUpdate();

		for (auto& child : children())
		{
			child->_resize();
		}
	}

	void Widget::forceGeometryChange(const Geometry2D& p_geometry)
	{
		if (_geometry == p_geometry)
			return;

		_geometry = p_geometry;
		_computeRatio();
		updateGeometry();
	}
	
	void Widget::setGeometry(const Geometry2D& p_geometry)
	{
		if (_geometry == p_geometry)
			return;

		_geometry = p_geometry;
		_computeRatio();
		_needGeometryChange = true;
	}
	
	void Widget::forceGeometryChange(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
	{
		forceGeometryChange({ p_anchor, p_size });
	}
	
	void Widget::setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
	{
		setGeometry({ p_anchor, p_size });
	}
	
	void Widget::updateGeometry()
	{
		_onGeometryChange();
		_needGeometryChange = false;
		for (auto& child : children())
		{
			child->requireGeometryUpdate();
		}
	}

	void Widget::requireGeometryUpdate()
	{
		_needGeometryChange = true;
		for (auto& child : children())
		{
			child->requireGeometryUpdate();
		}
	}

	const Geometry2D& Widget::geometry() const
	{
		return (_geometry);
	}

	const Viewport& Widget::viewport() const
	{
		return (_viewport);
	}

	void Widget::_onGeometryChange()
	{

	}

	void Widget::_onPaintEvent(const spk::PaintEvent& p_event)
	{

	}

	spk::Geometry2D::Point Widget::_computeAbsoluteAnchor()
	{
		spk::Geometry2D::Point result = { 0, 0 };
		const Widget* tmp = this;

		while (tmp->parent() != nullptr)
		{
			result += tmp->geometry().anchor;
			tmp = static_cast<const Widget*>(tmp->parent());
		}

		return (result);
	}

	void Widget::_computeViewport()
	{
		spk::Geometry2D::Point topLeft = _computeAbsoluteAnchor();
		spk::Geometry2D::Size rightDown = geometry().size + spk::Geometry2D::Size(topLeft.x, topLeft.y);
		
		if (parent() != nullptr)
		{
			topLeft = Geometry2D::Point::max(topLeft, static_cast<const Widget*>(parent())->viewport().geometry().anchor);
		}

		if (parent() != nullptr)
		{
			spk::Geometry2D::Size tmpSize = static_cast<const Widget*>(parent())->geometry().size;
			spk::Geometry2D::Size tmpAnchor = { static_cast<const Widget*>(parent())->geometry().x, static_cast<const Widget*>(parent())->geometry().y };
			rightDown = Geometry2D::Size::min(rightDown, tmpSize + tmpAnchor);
		}

		spk::Geometry2D::Size size = { rightDown.x - topLeft.x, rightDown.y - topLeft.y };

		_viewport.setGeometry({ topLeft, size });
	}

	void Widget::onPaintEvent(const spk::PaintEvent& p_event)
	{
		if (isActive() == false)
			return;

		if (_needGeometryChange == true)
		{
			updateGeometry();
			_computeViewport();
		}

		if (WidgetConstants == nullptr)
		{
			try
			{
				WidgetConstants = &(spk::Pipeline::constants(L"spk::WidgetConstants"));
			}
			catch (...)
			{
				WidgetConstants = nullptr;
			}
		}

		if (parent() != nullptr)
		{
			static_cast<const spk::Widget*>(parent())->viewport().apply();

			if (WidgetConstants != nullptr)
			{
				(*WidgetConstants)[L"layer"] = _layer;
				WidgetConstants->validate();
			}
		}
		
		_onPaintEvent(p_event);

		for (auto& child : children())
		{
			if (child->isActive() == true)
			{
				_viewport.apply();
				child->onPaintEvent(p_event);
			}
		}
	}

	void Widget::_onUpdateEvent(const spk::UpdateEvent& p_event)
	{

	}

	void Widget::onUpdateEvent(const spk::UpdateEvent& p_event)
	{
		if (isActive() == false)
			return;

		for (auto& child : children())
		{
			child->onUpdateEvent(p_event);
		}

		_onUpdateEvent(p_event);
	}

	void Widget::_onKeyboardEvent(const spk::KeyboardEvent& p_event)
	{

	}

	void Widget::onKeyboardEvent(const spk::KeyboardEvent& p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
			return;

		for (auto& child : children())
		{
			child->onKeyboardEvent(p_event);
		}

		_onKeyboardEvent(p_event);
	}

	void Widget::_onMouseEvent(const spk::MouseEvent& p_event)
	{

	}

	void Widget::onMouseEvent(const spk::MouseEvent& p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
			return;

		for (auto& child : children())
		{
			child->onMouseEvent(p_event);
		}

		_onMouseEvent(p_event);
	}
	
	void Widget::_onControllerEvent(const spk::ControllerEvent& p_event)
	{

	}
	
	void Widget::onControllerEvent(const spk::ControllerEvent& p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
			return;

		for (auto& child : children())
		{
			child->onControllerEvent(p_event);
		}

		_onControllerEvent(p_event);
	}
}