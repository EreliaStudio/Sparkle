#include "widget/spk_scalable_widget.hpp"

#include "structure/graphics/spk_window.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	ScalableWidget::ScalableWidget(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		spk::Widget(p_name, p_parent)
	{

	}

	const spk::Vector2UInt& ScalableWidget::minimumSize() const
	{
		return (_minimumSize);
	}
	
	const spk::Vector2UInt& ScalableWidget::maximumSize() const
	{
		return (_maximumSize);
	}

	void ScalableWidget::setMinimumSize(const spk::Vector2UInt& p_minimumSize)
	{
		_minimumSize = p_minimumSize;

		if (geometry().size.x < _minimumSize.x || geometry().size.y < _minimumSize.y)
		{
			setGeometry(geometry().anchor, spk::Vector2UInt::max(_minimumSize, geometry().size));
		}
	}

	void ScalableWidget::setMaximumSize(const spk::Vector2UInt& p_maximumSize)
	{
		_maximumSize = p_maximumSize;
	}

	void ScalableWidget::place(const spk::Vector2Int& p_delta) 
	{
		Widget::place(p_delta);
		_updateAnchorAreas();
	}
	
	void ScalableWidget::move(const spk::Vector2Int& p_delta) 
	{
		Widget::move(p_delta);
		_updateAnchorAreas();
	}

	void ScalableWidget::forceGeometryChange(const Geometry2D& p_geometry)
	{
		Geometry2D newGeometry = p_geometry;

		newGeometry.size = spk::Vector2UInt::clamp(newGeometry.size, _minimumSize, _maximumSize);

		Widget::forceGeometryChange(newGeometry);
		_updateAnchorAreas();
	}

	void ScalableWidget::setGeometry(const Geometry2D& p_geometry)
	{
		Geometry2D newGeometry = p_geometry;

		newGeometry.size = spk::Vector2UInt::clamp(newGeometry.size, _minimumSize, _maximumSize);

		Widget::setGeometry(newGeometry);
		_updateAnchorAreas();
	}
	
	void ScalableWidget::forceGeometryChange(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
	{
		Vector2UInt newSize = spk::Vector2UInt::clamp(p_size, _minimumSize, _maximumSize);

		Widget::forceGeometryChange(p_anchor, newSize);
		_updateAnchorAreas();
	}
	
	void ScalableWidget::setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
	{
		Vector2UInt newSize = spk::Vector2UInt::clamp(p_size, _minimumSize, _maximumSize);

		Widget::setGeometry(p_anchor, newSize);
		_updateAnchorAreas();
	}

	void ScalableWidget::_updateAnchorAreas()
	{
		constexpr int offset = 5;

		_topAnchorArea = {
			{ geometry().anchor.x - offset,         geometry().anchor.y - offset },
			{ geometry().size.x + offset * 2,      offset * 2 }
		};

		_leftAnchorArea = {
			{ geometry().anchor.x - offset,         geometry().anchor.y - offset },
			{ offset * 2,                          geometry().size.y + offset * 2 }
		};

		_rightAnchorArea = {
			{ geometry().anchor.x + geometry().size.x - offset,  geometry().anchor.y - offset },
			{ offset * 2,                                       geometry().size.y + offset * 2 }
		};

		_downAnchorArea = {
			{ geometry().anchor.x - offset,         geometry().anchor.y + geometry().size.y - offset },
			{ geometry().size.x + offset * 2,      offset * 2 }
		};
	}

	void ScalableWidget::_onMouseEvent(spk::MouseEvent& p_event)
	{
		switch (p_event.type)
		{
		case spk::MouseEvent::Type::Motion:
		{
			if (_isTopResizing == false && _isDownResizing == false &&
				_isLeftResizing == false && _isRightResizing == false)
			{
				bool topAnchor    = _topAnchorArea.contains(p_event.mouse->position);
				bool downAnchor   = _downAnchorArea.contains(p_event.mouse->position);
				bool leftAnchor   = _leftAnchorArea.contains(p_event.mouse->position);
				bool rightAnchor  = _rightAnchorArea.contains(p_event.mouse->position);

				if ((topAnchor && leftAnchor) || (downAnchor && rightAnchor))
				{
					p_event.window->setCursor(L"ResizeNWSE");
				}
				else if ((topAnchor && rightAnchor) || (downAnchor && leftAnchor))
				{
					p_event.window->setCursor(L"ResizeNESW");
				}
				else if (topAnchor || downAnchor)
				{
					p_event.window->setCursor(L"ResizeNS");
				}
				else if (leftAnchor || rightAnchor)
				{
					p_event.window->setCursor(L"ResizeWE");
				}
				else
				{
					p_event.window->setCursor(L"Arrow");
				}
			}
			else
			{
				spk::Vector2Int delta = p_event.mouse->position - _positionDelta;
				spk::Geometry2D newGeometry = _baseGeometry;

				if (_isTopResizing)
				{
					float nextSize = static_cast<float>(_baseGeometry.size.y) - static_cast<float>(delta.y);

					if (nextSize < _minimumSize.y)
						newGeometry.anchor.y += static_cast<int>(_baseGeometry.size.y) - _minimumSize.y;
					else if (nextSize > _maximumSize.y)
						newGeometry.anchor.y += static_cast<int>(_baseGeometry.size.y) - _maximumSize.y;
					else
						newGeometry.anchor.y += static_cast<int>(_baseGeometry.size.y) - nextSize;

					newGeometry.size.y = _baseGeometry.anchor.y + _baseGeometry.size.y - newGeometry.anchor.y;
				}

				else if (_isDownResizing)
				{
					float nextSize = static_cast<float>(_baseGeometry.size.y) + static_cast<float>(delta.y);

					if (nextSize < _minimumSize.y)
						newGeometry.size.y = _minimumSize.y;
					else if (nextSize > _maximumSize.y)
						newGeometry.size.y = _maximumSize.y;
					else 
						newGeometry.size.y = nextSize;
				}

				if (_isLeftResizing)
				{
					float nextSize = static_cast<float>(_baseGeometry.size.x) - static_cast<float>(delta.x);

					if (nextSize < _minimumSize.x)
						newGeometry.anchor.x += static_cast<int>(_baseGeometry.size.x) - _minimumSize.x;
					else if (nextSize > _maximumSize.x)
						newGeometry.anchor.x += static_cast<int>(_baseGeometry.size.x) - _maximumSize.x;
					else
						newGeometry.anchor.x += static_cast<int>(_baseGeometry.size.x) - nextSize;

					newGeometry.size.x = _baseGeometry.anchor.x + _baseGeometry.size.x - newGeometry.anchor.x;
				}

				else if (_isRightResizing)
				{
					float nextSize = static_cast<float>(_baseGeometry.size.x) + static_cast<float>(delta.x);

					if (nextSize < _minimumSize.x)
						newGeometry.size.x = _minimumSize.x;
					else if (nextSize > _maximumSize.x)
						newGeometry.size.x = _maximumSize.x;
					else 
						newGeometry.size.x = nextSize;
				}

				if (newGeometry != geometry())
				{
					setGeometry(newGeometry);
					p_event.requestPaint();
					p_event.consume();
				}
			}
			break;
		}
		case spk::MouseEvent::Type::Press:
		{
			if (p_event.button == spk::Mouse::Button::Left)
			{
				if (_topAnchorArea.contains(p_event.mouse->position))
				{
					_isTopResizing = true;
				}
				else if (_downAnchorArea.contains(p_event.mouse->position))
				{
					_isDownResizing = true;
				}

				if (_leftAnchorArea.contains(p_event.mouse->position))
				{
					_isLeftResizing = true;
				}
				else if (_rightAnchorArea.contains(p_event.mouse->position))
				{
					_isRightResizing = true;
				}

				if (_isTopResizing || _isDownResizing || _isLeftResizing || _isRightResizing)
				{
					_positionDelta = p_event.mouse->position;
					_baseGeometry = geometry();

					p_event.consume();
				}
			}
			break;
		}
		case spk::MouseEvent::Type::Release:
		{
			if (p_event.button == spk::Mouse::Button::Left)
			{
				_isTopResizing   = false;
				_isLeftResizing  = false;
				_isRightResizing = false;
				_isDownResizing  = false;

				p_event.window->setCursor(L"Arrow");
			}
			break;
		}
		}
	}
}
