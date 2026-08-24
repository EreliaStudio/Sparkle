#include "ui/widget/scalable_widget.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace
{
	bool contains(std::int64_t value, std::int64_t minimum, std::int64_t maximum) noexcept
	{
		return value >= minimum && value <= maximum;
	}
}

namespace spk
{
	ScalableWidget::ScalableWidget(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		const float unlimitedSize = static_cast<float>(std::numeric_limits<std::size_t>::max());
		setMaximalSize({unlimitedSize, unlimitedSize});
		_sizeHintEditionContract = subscribeToSizeHintEdition([this](ResizeableTrait *) {
			_applyGeometryConstraints();
		});
	}

	unsigned int ScalableWidget::_dimension(float value) noexcept
	{
		if (!std::isfinite(value))
		{
			return value > 0.0f ? std::numeric_limits<unsigned int>::max() : 0;
		}
		if (value <= 0.0f)
		{
			return 0;
		}
		const float maximum = static_cast<float>(std::numeric_limits<unsigned int>::max());
		if (value >= maximum)
		{
			return std::numeric_limits<unsigned int>::max();
		}
		return static_cast<unsigned int>(std::round(value));
	}

	Rect2D ScalableWidget::_constrainedGeometry(const Rect2D &candidate) const noexcept
	{
		const Vector2UInt minimum{
			_dimension(minimalSize().x),
			_dimension(minimalSize().y)};
		const Vector2UInt maximum{
			std::max(minimum.x, _dimension(maximalSize().x)),
			std::max(minimum.y, _dimension(maximalSize().y))};
		return {
			candidate.anchor,
			Vector2UInt{
				std::clamp(candidate.width, minimum.x, maximum.x),
				std::clamp(candidate.height, minimum.y, maximum.y)}};
	}

	Rect2D ScalableWidget::_resizedGeometry(const Vector2Int &position) const noexcept
	{
		const std::int64_t deltaX = static_cast<std::int64_t>(position.x) - _resizeStartPosition.x;
		const std::int64_t deltaY = static_cast<std::int64_t>(position.y) - _resizeStartPosition.y;
		std::int64_t requestedWidth = _baseGeometry.width;
		std::int64_t requestedHeight = _baseGeometry.height;
		if (containsEdge(_activeEdges, Left))
		{
			requestedWidth -= deltaX;
		}
		else if (containsEdge(_activeEdges, Right))
		{
			requestedWidth += deltaX;
		}
		if (containsEdge(_activeEdges, Top))
		{
			requestedHeight -= deltaY;
		}
		else if (containsEdge(_activeEdges, Bottom))
		{
			requestedHeight += deltaY;
		}

		const auto requestedDimension = [](std::int64_t value) {
			return static_cast<unsigned int>(std::clamp<std::int64_t>(value, 0, std::numeric_limits<unsigned int>::max()));
		};
		Rect2D result = _constrainedGeometry({_baseGeometry.anchor, Vector2UInt{requestedDimension(requestedWidth), requestedDimension(requestedHeight)}});
		if (containsEdge(_activeEdges, Left))
		{
			result.x = static_cast<int>(static_cast<std::int64_t>(_baseGeometry.x) + _baseGeometry.width - result.width);
		}
		if (containsEdge(_activeEdges, Top))
		{
			result.y = static_cast<int>(static_cast<std::int64_t>(_baseGeometry.y) + _baseGeometry.height - result.height);
		}
		return result;
	}

	ScalableWidget::Edges ScalableWidget::_edgesAt(const Vector2Int &position) const noexcept
	{
		if (geometry().width == 0 || geometry().height == 0)
		{
			return None;
		}
		const Rect2D &viewport = viewRegion().viewport;
		const std::int64_t left = viewport.x;
		const std::int64_t right = left + viewport.width;
		const std::int64_t top = viewport.y;
		const std::int64_t bottom = top + viewport.height;
		const std::int64_t x = position.x;
		const std::int64_t y = position.y;
		const std::int64_t offset = _grabOffset;

		Edges result = None;
		if (contains(y, top - offset, bottom + offset))
		{
			const std::int64_t leftDistance = std::abs(x - left);
			const std::int64_t rightDistance = std::abs(x - right);
			if (leftDistance <= offset || rightDistance <= offset)
			{
				result |= leftDistance <= rightDistance ? Left : Right;
			}
		}
		if (contains(x, left - offset, right + offset))
		{
			const std::int64_t topDistance = std::abs(y - top);
			const std::int64_t bottomDistance = std::abs(y - bottom);
			if (topDistance <= offset || bottomDistance <= offset)
			{
				result |= topDistance <= bottomDistance ? Top : Bottom;
			}
		}
		return result;
	}

	void ScalableWidget::_applyGeometryConstraints()
	{
		const Rect2D constrained = _constrainedGeometry(geometry());
		if (constrained != geometry())
		{
			setGeometry(constrained);
		}
	}

	void ScalableWidget::_beginResize(EventBase &event, Mouse::Button button, const Vector2Int &position)
	{
		if (button != Mouse::Button::Left || _activeEdges != None)
		{
			return;
		}
		const Edges edges = _edgesAt(position);
		if (edges == None)
		{
			return;
		}
		_activeEdges = edges;
		_hoveredEdges = edges;
		_baseGeometry = geometry();
		_resizeStartPosition = position;
		event.takeFocus(FocusMode::Channel::Mouse, this);
		event.consumed = true;
	}

	void ScalableWidget::_endResize(EventBase &event)
	{
		if (_activeEdges == None)
		{
			return;
		}
		_activeEdges = None;
		_hoveredEdges = None;
		event.releaseFocus(FocusMode::Channel::Mouse, this);
		event.consumed = true;
	}

	void ScalableWidget::_setGeometryWithoutConstraints(const Rect2D &geometry)
	{
		_applyingConstraints = true;
		Widget::setGeometry(geometry);
		_applyingConstraints = false;
	}

	void ScalableWidget::_onGeometryChange()
	{
		if (_applyingConstraints)
		{
			return;
		}
		const Rect2D constrained = _constrainedGeometry(geometry());
		if (constrained != geometry())
		{
			_applyingConstraints = true;
			Widget::setGeometry(constrained);
			_applyingConstraints = false;
		}
	}

	void ScalableWidget::_onWindowFocusLostEvent(WindowFocusLostEvent &event)
	{
		_endResize(event);
	}

	void ScalableWidget::_onMouseLeftEvent(MouseLeftEvent &)
	{
		if (_activeEdges == None)
		{
			_hoveredEdges = None;
		}
	}

	void ScalableWidget::_onMouseMovedEvent(MouseMovedEvent &event)
	{
		if (_activeEdges == None)
		{
			_hoveredEdges = _edgesAt(event.device.position);
			// TODO: Apply the matching resize cursor once Sparkle exposes cursor-shape requests.
			return;
		}

		const Rect2D resized = _resizedGeometry(event.device.position);
		if (resized != geometry())
		{
			setGeometry(resized);
			event.consumed = true;
		}
	}

	void ScalableWidget::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		_beginResize(event, event.record.button, event.device.position);
	}

	void ScalableWidget::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event)
	{
		if (event.record.button == Mouse::Button::Left)
		{
			_endResize(event);
		}
	}

	void ScalableWidget::_onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &event)
	{
		_beginResize(event, event.record.button, event.device.position);
	}

	void ScalableWidget::setGeometry(const Rect2D &geometry)
	{
		Widget::setGeometry(_constrainedGeometry(geometry));
	}

	void ScalableWidget::resize(const Rect2D &geometry)
	{
		Widget::resize(_constrainedGeometry(geometry));
	}

	void ScalableWidget::setGrabOffset(unsigned int offset) noexcept
	{
		_grabOffset = offset;
	}

	bool ScalableWidget::isResizing() const noexcept
	{
		return _activeEdges != None;
	}

	ScalableWidget::Edges ScalableWidget::activeEdges() const noexcept
	{
		return _activeEdges;
	}

	ScalableWidget::Edges ScalableWidget::hoveredEdges() const noexcept
	{
		return _hoveredEdges;
	}

	unsigned int ScalableWidget::grabOffset() const noexcept
	{
		return _grabOffset;
	}

	bool ScalableWidget::containsEdge(Edges edges, Edge edge) noexcept
	{
		return (edges & static_cast<Edges>(edge)) != 0;
	}
}
