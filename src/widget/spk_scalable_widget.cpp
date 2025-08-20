#include "widget/spk_scalable_widget.hpp"
#include "spk_debug_macro.hpp"
#include "structure/graphics/spk_window.hpp"

namespace spk
{
	ScalableWidget::ScalableWidget(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent) :
		spk::Widget(p_name, p_parent)
	{
	}

	spk::Vector2UInt ScalableWidget::minimalSize() const
	{
		return _minimumSize;
	}
	spk::Vector2UInt ScalableWidget::maximalSize() const
	{
		return _maximumSize;
	}

	void ScalableWidget::setMinimumSize(const spk::Vector2UInt &p_minimumSize)
	{
		_minimumSize = p_minimumSize;
		if (geometry().size.x < _minimumSize.x || geometry().size.y < _minimumSize.y)
		{
			setGeometry(viewport().geometry().anchor, spk::Vector2UInt::max(_minimumSize, geometry().size));
		}
	}

	void ScalableWidget::setMaximumSize(const spk::Vector2UInt &p_maximumSize)
	{
		_maximumSize = p_maximumSize;
	}

	void ScalableWidget::place(const spk::Vector2Int &p_delta)
	{
		Widget::place(p_delta);
		_updateAnchorAreas();
	}
	void ScalableWidget::move(const spk::Vector2Int &p_delta)
	{
		Widget::move(p_delta);
		_updateAnchorAreas();
	}

	void ScalableWidget::forceGeometryChange(const Geometry2D &p_geometry)
	{
		Geometry2D g = p_geometry;
		g.size = spk::Vector2UInt::clamp(g.size, _minimumSize, _maximumSize);
		Widget::forceGeometryChange(g);
		_updateAnchorAreas();
	}

	void ScalableWidget::setGeometry(const Geometry2D &p_geometry)
	{
		Geometry2D g = p_geometry;
		g.size = spk::Vector2UInt::clamp(g.size, _minimumSize, _maximumSize);
		Widget::setGeometry(g);
		_updateAnchorAreas();
	}

	void ScalableWidget::forceGeometryChange(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size)
	{
		Vector2UInt s = spk::Vector2UInt::clamp(p_size, _minimumSize, _maximumSize);
		Widget::forceGeometryChange(p_anchor, s);
		_updateAnchorAreas();
	}

	void ScalableWidget::setGeometry(const spk::Vector2Int &p_anchor, const spk::Vector2UInt &p_size)
	{
		Vector2UInt s = spk::Vector2UInt::clamp(p_size, _minimumSize, _maximumSize);
		Widget::setGeometry(p_anchor, s);
		_updateAnchorAreas();
	}

	void ScalableWidget::_updateAnchorAreas()
	{
		constexpr int offset = 5;
		const auto &vp = viewport().geometry();
		_topAnchorArea = {{vp.anchor.x - offset, vp.anchor.y - offset}, {geometry().size.x + offset * 2, offset * 2}};
		_leftAnchorArea = {{vp.anchor.x - offset, vp.anchor.y - offset}, {offset * 2, geometry().size.y + offset * 2}};
		_rightAnchorArea = {{vp.anchor.x + geometry().size.x - offset, vp.anchor.y - offset}, {offset * 2, geometry().size.y + offset * 2}};
		_downAnchorArea = {{vp.anchor.x - offset, vp.anchor.y + geometry().size.y - offset}, {geometry().size.x + offset * 2, offset * 2}};
	}

	// ---- helpers ------------------------------------------------------------

	uint8_t ScalableWidget::_hoverEdges(const spk::Vector2Int &p_mousePos) const
	{
		uint8_t edges = Edge::None;
		if (_topAnchorArea.contains(p_mousePos))
		{
			edges |= Edge::Top;
		}
		if (_downAnchorArea.contains(p_mousePos))
		{
			edges |= Edge::Bottom;
		}
		if (_leftAnchorArea.contains(p_mousePos))
		{
			edges |= Edge::Left;
		}
		if (_rightAnchorArea.contains(p_mousePos))
		{
			edges |= Edge::Right;
		}
		return edges;
	}

	void ScalableWidget::_setCursorForEdges(uint8_t p_edges, spk::SafePointer<Window> p_window)
	{
		// Diagonals first
		const bool top = (p_edges & Edge::Top) != 0;
		const bool bottom = (p_edges & Edge::Bottom) != 0;
		const bool left = (p_edges & Edge::Left) != 0;
		const bool right = (p_edges & Edge::Right) != 0;

		if ((top && left) || (bottom && right))
		{
			p_window->setCursor(L"ResizeNWSE");
			return;
		}
		if ((top && right) || (bottom && left))
		{
			p_window->setCursor(L"ResizeNESW");
			return;
		}
		if (top || bottom)
		{
			p_window->setCursor(L"ResizeNS");
			return;
		}
		if (left || right)
		{
			p_window->setCursor(L"ResizeWE");
			return;
		}
		p_window->setCursor(L"Arrow");
	}

	void ScalableWidget::_beginResize(uint8_t p_edges, const spk::Vector2Int &p_startPos)
	{
		_activeEdges = p_edges;
		_positionDelta = p_startPos;
		_baseGeometry = geometry();
	}

	void ScalableWidget::_endResize(spk::SafePointer<Window> p_window)
	{
		_activeEdges = Edge::None;
		p_window->setCursor(L"Arrow");
	}

	// Compute clamped 1D resize for one axis
	static inline void compute1DResize(
		int p_baseAnchor, int p_baseSize, int p_delta, int p_minSize, int p_maxSize, bool p_fromMinSide, int &p_outAnchor, int &p_outSize)
	{
		// fromMinSide: true => drag min side (left/top), false => drag max side (right/bottom)
		if (p_fromMinSide)
		{
			const float proposed = static_cast<float>(p_baseSize) - static_cast<float>(p_delta);
			float clamped = proposed;
			if (proposed < static_cast<float>(p_minSize))
			{
				clamped = static_cast<float>(p_minSize);
			}
			else if (proposed > static_cast<float>(p_maxSize))
			{
				clamped = static_cast<float>(p_maxSize);
			}

			// anchor shifts so that far edge stays put
			p_outAnchor = p_baseAnchor + (p_baseSize - static_cast<int>(clamped));
			p_outSize = (p_baseAnchor + p_baseSize) - p_outAnchor; // keep far edge fixed
		}
		else
		{
			const float proposed = static_cast<float>(p_baseSize) + static_cast<float>(p_delta);
			float clamped = proposed;
			if (proposed < static_cast<float>(p_minSize))
			{
				clamped = static_cast<float>(p_minSize);
			}
			else if (proposed > static_cast<float>(p_maxSize))
			{
				clamped = static_cast<float>(p_maxSize);
			}

			p_outAnchor = p_baseAnchor;
			p_outSize = static_cast<int>(clamped);
		}
	}

	spk::Geometry2D ScalableWidget::_computeResizedGeometry(const spk::Vector2Int &p_currentPos) const
	{
		const spk::Vector2Int delta = p_currentPos - _positionDelta;

		spk::Geometry2D g = _baseGeometry;

		// Vertical
		if ((_activeEdges & Edge::Top) != 0)
		{
			compute1DResize(
				_baseGeometry.anchor.y,
				_baseGeometry.size.y,
				delta.y,
				static_cast<int>(_minimumSize.y),
				static_cast<int>(_maximumSize.y),
				/*fromMinSide*/ true,
				g.anchor.y,
				reinterpret_cast<int &>(g.size.y));
		}
		else if ((_activeEdges & Edge::Bottom) != 0)
		{
			compute1DResize(
				_baseGeometry.anchor.y,
				_baseGeometry.size.y,
				delta.y,
				static_cast<int>(_minimumSize.y),
				static_cast<int>(_maximumSize.y),
				/*fromMinSide*/ false,
				g.anchor.y,
				reinterpret_cast<int &>(g.size.y));
		}

		// Horizontal
		if ((_activeEdges & Edge::Left) != 0)
		{
			compute1DResize(
				_baseGeometry.anchor.x,
				_baseGeometry.size.x,
				delta.x,
				static_cast<int>(_minimumSize.x),
				static_cast<int>(_maximumSize.x),
				/*fromMinSide*/ true,
				g.anchor.x,
				reinterpret_cast<int &>(g.size.x));
		}
		else if ((_activeEdges & Edge::Right) != 0)
		{
			compute1DResize(
				_baseGeometry.anchor.x,
				_baseGeometry.size.x,
				delta.x,
				static_cast<int>(_minimumSize.x),
				static_cast<int>(_maximumSize.x),
				/*fromMinSide*/ false,
				g.anchor.x,
				reinterpret_cast<int &>(g.size.x));
		}

		// Final clamp (safety)
		g.size = spk::Vector2UInt::clamp(g.size, _minimumSize, _maximumSize);
		return g;
	}

	// ---- drastically simplified handler ------------------------------------

	void ScalableWidget::_onMouseEvent(spk::MouseEvent &p_event)
	{
		switch (p_event.type)
		{
		case spk::MouseEvent::Type::Motion:
		{
			if (_activeEdges == Edge::None)
			{
				const uint8_t hover = _hoverEdges(p_event.mouse->position());
				_setCursorForEdges(hover, p_event.window);
			}
			else
			{
				const spk::Geometry2D newGeom = _computeResizedGeometry(p_event.mouse->position());
				if (newGeom != geometry())
				{
					setGeometry(newGeom);
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
				const uint8_t edges = _hoverEdges(p_event.mouse->position());
				if (edges != Edge::None)
				{
					_beginResize(edges, p_event.mouse->position());
					p_event.consume();
				}
			}
			break;
		}

		case spk::MouseEvent::Type::Release:
		{
			if (p_event.button == spk::Mouse::Button::Left)
			{
				_endResize(p_event.window);
			}
			break;
		}

		default:
		{
			break;
		}
		}
	}
}
