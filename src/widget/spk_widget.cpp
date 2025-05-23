#include "widget/spk_widget.hpp"
#include "spk_generated_resources.hpp"

#include "utils/spk_string_utils.hpp"

#include "structure/graphics/renderer/spk_color_renderer.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	spk::SpriteSheet Widget::_defaultIconset = spk::SpriteSheet::fromRawData(SPARKLE_GET_RESOURCE("resources/textures/defaultIconset.png"),
																			 spk::Vector2Int(10, 10), spk::SpriteSheet::Filtering::Linear);

	spk::SafePointer<const spk::SpriteSheet> Widget::defaultIconset()
	{
		return (&_defaultIconset);
	}

	spk::Font Widget::_defaultFont = spk::Font::fromRawData(SPARKLE_GET_RESOURCE("resources/font/arial.ttf"), Font::Filtering::Nearest);

	spk::SafePointer<spk::Font> Widget::defaultFont()
	{
		return (&_defaultFont);
	}

	spk::SpriteSheet Widget::_defaultNineSlice = spk::SpriteSheet::fromRawData(SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice.png"),
																			   spk::Vector2UInt(3, 3), SpriteSheet::Filtering::Linear);

	spk::SafePointer<const spk::SpriteSheet> Widget::defaultNineSlice()
	{
		return (&_defaultNineSlice);
	}

	Widget::Widget(const std::wstring &p_name) :
		_name(p_name),
		_parent(nullptr),
		_layer(0)
	{
	}

	Widget::Widget(const std::wstring &p_name, spk::SafePointer<Widget> p_parent) :
		Widget(p_name)
	{
		if (p_parent != nullptr)
		{
			p_parent->addChild(this);
		}
	}

	Widget::~Widget()
	{
		releaseFocus();

		if (_parent != nullptr)
		{
			_parent->removeChild(this);
		}

		for (auto &child : _managedChildren)
		{
			delete child;
		}
	}

	const std::wstring &Widget::name() const
	{
		return (_name);
	}

	void Widget::addChild(spk::SafePointer<Widget> p_child)
	{
		spk::InherenceObject<Widget>::addChild(p_child);
		p_child->setLayer(layer() + 1);
		p_child->_viewport.setWindowSize(_viewport.windowSize());
	}

	bool Widget::isPointed(const spk::Vector2Int &p_pointerPosition) const
	{
		return (viewport().geometry().contains(p_pointerPosition));
	}

	bool Widget::isPointed(const spk::Mouse &p_mouse) const
	{
		return (isPointed(p_mouse.position));
	}

	bool Widget::isPointed(const spk::SafePointer<const spk::Mouse> &p_mouse) const
	{
		if (p_mouse == nullptr)
		{
			return (false);
		}
		return (isPointed(*p_mouse));
	}

	void Widget::setLayer(const float &p_layer)
	{
		float delta = (p_layer - _layer);
		for (auto &child : children())
		{
			static_cast<Widget *>(child)->setLayer(static_cast<Widget *>(child)->layer() + delta);
		}
		_layer = (parent() == nullptr ? 0 : parent()->layer()) + p_layer;
		requireGeometryUpdate();
	}

	const float &Widget::layer() const
	{
		return (_layer);
	}

	spk::SafePointer<Widget> Widget::focusedWidget(FocusType p_focusType)
	{
		return (_focusedWidgets[static_cast<int>(p_focusType)]);
	}

	void Widget::requestPaint()
	{
		_requestedPaint = true;
	}

	void Widget::takeFocus(FocusType p_focusType)
	{
		_focusedWidgets[static_cast<int>(p_focusType)] = this;
	}

	void Widget::releaseFocus(FocusType p_focusType)
	{
		if (_focusedWidgets[static_cast<int>(p_focusType)] == this)
		{
			_focusedWidgets[static_cast<int>(p_focusType)] = nullptr;
		}
	}

	bool Widget::hasFocus(FocusType p_focusType)
	{
		return (focusedWidget(p_focusType) == this);
	}

	void Widget::takeFocus()
	{
		takeFocus(FocusType::KeyboardFocus);
		takeFocus(FocusType::MouseFocus);
		takeFocus(FocusType::ControllerFocus);
	}

	void Widget::releaseFocus()
	{
		releaseFocus(FocusType::KeyboardFocus);
		releaseFocus(FocusType::MouseFocus);
		releaseFocus(FocusType::ControllerFocus);
	}

	void Widget::_computeRatio()
	{
		spk::Vector2Int parentSize = (parent() == nullptr ? _viewport.windowSize() : parent()->_geometry.size);

		_anchorRatio = {static_cast<float>(_geometry.anchor.x) / static_cast<float>(parentSize.x),
						static_cast<float>(_geometry.anchor.y) / static_cast<float>(parentSize.y)};
		_sizeRatio = {static_cast<float>(_geometry.size.x) / static_cast<float>(parentSize.x),
					  static_cast<float>(_geometry.size.y) / static_cast<float>(parentSize.y)};
	}

	void Widget::setGeometry(const Geometry2D &p_geometry)
	{
		_geometry = p_geometry;
		_viewport.setGeometry(p_geometry);
		_viewport.setWindowSize((parent() == nullptr ? p_geometry.size : parent()->viewport().windowSize()));
		_computeRatio();

		requireGeometryUpdate();
	}

	void Widget::forceGeometryChange(const Geometry2D &p_geometry)
	{
		setGeometry(p_geometry);
		_applyGeometryChange();
		_needGeometryChange = false;
	}

	void Widget::place(const spk::Vector2Int &p_delta)
	{
		_geometry.anchor = p_delta;
		requireGeometryUpdate();
	}

	void Widget::move(const spk::Vector2Int &p_delta)
	{
		_geometry.anchor += p_delta;
		requireGeometryUpdate();
	}

	void Widget::forceGeometryChange(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size)
	{
		forceGeometryChange({p_anchor, p_size});
	}

	void Widget::setGeometry(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size)
	{
		setGeometry({p_anchor, p_size});
	}

	void Widget::_applyGeometryChange()
	{
		_onGeometryChange();
		_computeViewport();
	}

	void Widget::_applyResize()
	{
		spk::Vector2Int parentSize = (parent() == nullptr ? _viewport.windowSize() : parent()->_geometry.size);

		forceGeometryChange({parentSize * _anchorRatio, parentSize * _sizeRatio});

		for (auto& child : children())
		{
			viewport().apply();
			child->_applyResize();
		}
	}

	void Widget::requireGeometryUpdate()
	{
		_needGeometryChange = true;
	}

	spk::Vector2UInt Widget::minimalSize() const
	{
		return {0, 0};
	}

	spk::Vector2UInt Widget::maximalSize() const
	{
		return {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};
	}

	const Geometry2D &Widget::geometry() const
	{
		return (_geometry);
	}

	const Viewport &Widget::viewport() const
	{
		return (_viewport);
	}

	void Widget::_onGeometryChange()
	{
	}

	void Widget::_onPaintEvent(spk::PaintEvent &p_event)
	{
	}

	spk::Geometry2D::Point Widget::absoluteAnchor()
	{
		spk::Geometry2D::Point result = {0, 0};
		const Widget *tmp = this;

		while (tmp->parent() != nullptr)
		{
			result += tmp->geometry().anchor;
			tmp = static_cast<const Widget *>(tmp->parent());
		}

		return (result);
	}

	void Widget::_computeViewport()
	{
		spk::Geometry2D::Point topLeft = absoluteAnchor();
		spk::Geometry2D::Size rightDown = geometry().size + spk::Geometry2D::Size(topLeft.x, topLeft.y);

		if (parent() != nullptr)
		{
			topLeft = Geometry2D::Point::max(topLeft, static_cast<const Widget *>(parent())->viewport().geometry().anchor);

			if (parent()->viewport().geometry().contains(topLeft) == false)
			{
				spk::Geometry2D::Size tmpSize = static_cast<const Widget *>(parent())->geometry().size;
				spk::Geometry2D::Size tmpAnchor = {static_cast<const Widget *>(parent())->geometry().x,
												   static_cast<const Widget *>(parent())->geometry().y};
				rightDown = Geometry2D::Size::min(rightDown, tmpSize + tmpAnchor);
			}
		}
		spk::Geometry2D::Size size = {rightDown.x - topLeft.x, rightDown.y - topLeft.y};

		_viewport.setGeometry({absoluteAnchor(), geometry().size});
	}

	void Widget::onPaintEvent(spk::PaintEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		if (_needGeometryChange == true)
		{
			try
			{
				_applyGeometryChange();
			}
			catch (const std::exception &e)
			{
				PROPAGATE_ERROR("Applying geometry change of [" + spk::StringUtils::wstringToString(name()) + "]", e);
			}
			catch (...)
			{
				GENERATE_ERROR("[" + spk::StringUtils::wstringToString(name()) + "] onGeometryChange - Unknow error type");
			}
			_needGeometryChange = false;
		}

		try
		{
			_onPaintEvent(p_event);
		}
		catch (const std::exception &e)
		{
			GENERATE_ERROR("[" + spk::StringUtils::wstringToString(name()) + "] onPaintEvent -  " + e.what());
		}
		catch (...)
		{
			GENERATE_ERROR("[" + spk::StringUtils::wstringToString(name()) + "] onPaintEvent - Unknow error type");
		}

		spk::PaintEvent childEvent = p_event;
		childEvent.geometry = geometry();

		for (auto &child : children())
		{
			if (child->isActive() == true)
			{
				try
				{
					_viewport.apply();
				}
				catch (...)
				{
					GENERATE_ERROR("Error while applying viewport of [" + spk::StringUtils::wstringToString(name()) +
											 "] with viewport of geometry [" + _viewport.geometry().to_string() + "]");
				}
				child->onPaintEvent(childEvent);
			}
		}
	}

	void Widget::_onUpdateEvent(spk::UpdateEvent &p_event)
	{
	}

	void Widget::onUpdateEvent(spk::UpdateEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		for (auto &child : children())
		{
			child->onUpdateEvent(p_event);
		}

		_onUpdateEvent(p_event);

		if (_requestedPaint == true)
		{
			p_event.requestPaint();
		}
	}

	void Widget::_onKeyboardEvent(spk::KeyboardEvent &p_event)
	{
	}

	void Widget::onKeyboardEvent(spk::KeyboardEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		for (auto &child : children())
		{
			child->onKeyboardEvent(p_event);
		}

		_onKeyboardEvent(p_event);
	}

	void Widget::_onMouseEvent(spk::MouseEvent &p_event)
	{
	}

	void Widget::onMouseEvent(spk::MouseEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		for (auto &child : children())
		{
			child->onMouseEvent(p_event);
		}

		_onMouseEvent(p_event);
	}

	void Widget::_onControllerEvent(spk::ControllerEvent &p_event)
	{
	}

	void Widget::onControllerEvent(spk::ControllerEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		for (auto &child : children())
		{
			child->onControllerEvent(p_event);
		}

		_onControllerEvent(p_event);
	}

	void Widget::_onTimerEvent(spk::TimerEvent &p_event)
	{
	}

	void Widget::onTimerEvent(spk::TimerEvent &p_event)
	{
		if (isActive() == false || p_event.consumed() == true)
		{
			return;
		}

		for (auto &child : children())
		{
			child->onTimerEvent(p_event);
		}

		_onTimerEvent(p_event);
	}
}