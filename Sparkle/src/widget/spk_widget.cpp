#include <cassert>

#include "application/spk_application.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	void Widget::_onRender()
	{
	}

	void Widget::_onUpdate()
	{
	}

	void Widget::_onGeometryChange()
	{
	}

	void Widget::render()
	{
		_computeViewport();

		if (_needGeometryUpdate == true)
		{
			_onGeometryChange();
			_needGeometryUpdate = false;
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

	void Widget::update()
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

	spk::Vector2Int Widget::_computeAbsoluteAnchor()
	{
		spk::Vector2Int result = 0;
		const Widget* tmp = this;

		while (tmp->parent() != nullptr)
		{
			result += tmp->anchor();
			tmp = tmp->parent();
		}

		return (result);
	}

	void Widget::_computeViewport()
	{
		spk::Vector2Int topLeft = _computeAbsoluteAnchor();
		spk::Vector2Int rightDown = size() + topLeft;

		if (parent() != nullptr)
			topLeft = spk::Vector2Int::max(topLeft, parent()->viewport().anchor());

		if (parent() != nullptr)
			rightDown = spk::Vector2Int::min(rightDown, parent()->viewport().anchor() + parent()->viewport().size());

		_viewport.setGeometry(topLeft, rightDown - topLeft);
	}

	Widget::Widget(Widget* p_parent) :
		Widget("Unnamed widget", p_parent)
	{
		
	}

	Widget::Widget(const std::string& p_name) :
		_name(p_name),
#ifndef NDEBUG
		_timeMetric(spk::Application::activeApplication()->profiler().metric<TimeMetric>(name())),
#endif
		_layer(0)
	{
		if (defaultParent != nullptr)
			defaultParent->addChild(this);
	}

	Widget::Widget(const std::string& p_name, Widget* p_parent) :
		Widget(p_name)
	{
		if (p_parent != nullptr)
			p_parent->addChild(this);
	}

	Widget::Widget(const spk::JSON::Object& p_object, Widget* p_parent) :
		Widget("Anonym widget", p_parent)
	{
		if (p_object.contains("Name") == true)
		{
			setName(p_object["Name"].as<std::string>());
		}
		if (p_object.contains("Active") == true)
		{
			setActivationState(p_object["Active"].as<bool>());
		}
	}
	
	Widget::~Widget()
	{
		if (parent() != nullptr)
		{
			transferChildrens(parent());
		}
	}

	void Widget::addChild(Widget* p_child)
	{
		TreeNode<Widget>::addChild(p_child);
		p_child->setLayer(layer() + 1);
	}

	void Widget::setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
	{
		if (p_anchor == _anchor && p_size == _size)
		{
			return;
		}
		_anchor = p_anchor;
		_size = p_size;

		_anchorRatio = (parent() != nullptr && parent()->size() != 0 ? static_cast<spk::Vector2>(_anchor) / static_cast<spk::Vector2>(parent()->size()) : 0);
		_sizeRatio = (parent() != nullptr && parent()->size() != 0 ? static_cast<spk::Vector2>(_size) / static_cast<spk::Vector2>(parent()->size()) : 1);
		_needGeometryUpdate = true;
	}

	void Widget::forceGeometryChange(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
	{
		setGeometry(p_anchor, p_size);
		_onGeometryChange();
		_needGeometryUpdate = false;
	}

	void Widget::resize(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
	{
		_anchor = p_anchor;
		_size = p_size;
		_needGeometryUpdate = true;
		
		for (auto& child : children())
		{
			child->resize(_size * child->_anchorRatio, _size * child->_sizeRatio);
		}
	}

	void Widget::setLayer(const float& p_layer)
	{
		_layer = p_layer;
		if (_layer > Viewport::_maxLayer)
			Viewport::_maxLayer = _layer;
	}

	void Widget::activateAll()
	{
		activate();
		for (auto& child : children())
		{
			child->activateAll();
		}
	}

	void Widget::deactivateAll()
	{
		deactivate();
		for (auto& child : children())
		{
			child->deactivateAll();
		}
	}

	bool Widget::hitTest(const Vector2& p_coord)
	{
		Vector2 p0 = _viewport.anchor();
		Vector2 p1 = p0 + _viewport.size();
		return Vector2::isInsideRectangle(p_coord, p0, p1);
	}

	const std::string& Widget::name() const
	{
		return (_name);
	}

	void Widget::setName(const std::string& p_name)
	{
		_name = p_name;
	}

	const spk::Vector2Int& Widget::anchor() const
	{
		return (_anchor);
	}

	const float& Widget::layer() const
	{
		return (_layer);
	}

	const spk::Vector2UInt& Widget::size() const
	{
		return (_size);
	}

	const spk::Viewport& Widget::viewport() const
	{
		return (_viewport);
	}
}
