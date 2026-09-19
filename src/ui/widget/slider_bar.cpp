#include "ui/widget/slider_bar.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace spk
{
	SliderBar::SliderBar(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_background(this->name() + ".background", this),
		_body(this->name() + ".body", this)
	{
		applyStyle(defaultStyle);
		_background.setZOrder(0.0f);
		_body.setZOrder(1.0f);
		_updateSizeHint();
		activate();
	}

	void SliderBar::applyStyle(const Style &style)
	{
		if (style.darkerNineSlice != nullptr)
		{
			_background.setSpriteSheet(style.darkerNineSlice.get());
		}
		if (style.sliderBody != nullptr)
		{
			_body.setSpriteSheet(style.sliderBody.get());
		}
		_background.setCornerSize(style.sliderBarBackgroundCornerSize);
		_body.setCornerSize(style.sliderBarBodyCornerSize);
		_updateSizeHint();
		_onGeometryChange();
	}

	unsigned int SliderBar::_primaryLength() const noexcept
	{
		return _orientation == Orientation::Horizontal ? geometry().width : geometry().height;
	}

	unsigned int SliderBar::_crossLength() const noexcept
	{
		return _orientation == Orientation::Horizontal ? geometry().height : geometry().width;
	}

	unsigned int SliderBar::_bodyLength() const noexcept
	{
		const unsigned int primaryLength = _primaryLength();
		const auto scaledLength = static_cast<unsigned int>(std::lround(static_cast<float>(primaryLength) * _scale));
		return std::min(primaryLength, std::max(scaledLength, _crossLength()));
	}

	unsigned int SliderBar::_travelLength() const noexcept
	{
		return _primaryLength() - _bodyLength();
	}

	int SliderBar::_primaryCoordinate(const Vector2Int &position) const noexcept
	{
		return _orientation == Orientation::Horizontal ? position.x : position.y;
	}

	void SliderBar::_beginDrag(MouseButtonPressedEvent &event)
	{
		_dragging = true;
		_dragStartPosition = event.device.position;
		_dragStartRatio = _ratio;
		event.takeFocus(FocusMode::Channel::Mouse, this);
		event.consumed = true;
	}

	void SliderBar::_updateBodyGeometry()
	{
		const unsigned int bodyLength = _bodyLength();
		const auto bodyPosition = static_cast<int>(std::lround(static_cast<float>(_travelLength()) * _ratio));
		if (_orientation == Orientation::Horizontal)
		{
			_body.setGeometry({Vector2Int{bodyPosition, 0}, Vector2UInt{bodyLength, geometry().height}});
		}
		else
		{
			_body.setGeometry({Vector2Int{0, bodyPosition}, Vector2UInt{geometry().width, bodyLength}});
		}
	}

	void SliderBar::_updateSizeHint()
	{
		const auto maximum = [](const Vector2 &lhs, const Vector2 &rhs) {
			return Vector2{std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y)};
		};
		SizeHint hint = sizeHint();
		hint.minimal = maximum(_background.minimalSize(), _body.minimalSize());
		hint.preferred = maximum(_background.preferredSize(), _body.preferredSize());
		setSizeHint(hint);
	}

	void SliderBar::_onGeometryChange()
	{
		_background.setGeometry({Vector2Int{0, 0}, geometry().size});
		_updateBodyGeometry();
	}

	void SliderBar::_onWindowFocusLostEvent(WindowFocusLostEvent &event)
	{
		if (_dragging)
		{
			_dragging = false;
			event.releaseFocus(FocusMode::Channel::Mouse, this);
		}
	}

	void SliderBar::_onMouseMovedEvent(MouseMovedEvent &event)
	{
		if (!_dragging)
		{
			return;
		}

		const unsigned int travelLength = _travelLength();
		if (travelLength != 0)
		{
			const int pointerDelta = _primaryCoordinate(event.device.position) - _primaryCoordinate(_dragStartPosition);
			setRatio(_dragStartRatio + static_cast<float>(pointerDelta) / static_cast<float>(travelLength));
		}
		event.consumed = true;
	}

	void SliderBar::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (event.record.button != Mouse::Button::Left || !viewRegion().viewport.contains(event.device.position))
		{
			return;
		}

		if (!_body.viewRegion().viewport.contains(event.device.position))
		{
			const int pointerPosition = _primaryCoordinate(event.device.position) - _primaryCoordinate(viewRegion().viewport.anchor);
			const unsigned int travelLength = _travelLength();
			setRatio(travelLength == 0 ? 0.0f : (static_cast<float>(pointerPosition) - static_cast<float>(_bodyLength()) / 2.0f) / static_cast<float>(travelLength));
		}
		_beginDrag(event);
	}

	void SliderBar::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event)
	{
		if (event.record.button != Mouse::Button::Left || !_dragging)
		{
			return;
		}
		_dragging = false;
		event.releaseFocus(FocusMode::Channel::Mouse, this);
		event.consumed = true;
	}

	SliderBar::EditionContract SliderBar::subscribeToEdition(EditionCallback callback)
	{
		return _editionProvider.subscribe(std::move(callback));
	}

	void SliderBar::setOrientation(Orientation orientation)
	{
		if (_orientation == orientation)
		{
			return;
		}
		_orientation = orientation;
		_onGeometryChange();
	}

	void SliderBar::setScale(float scale)
	{
		if (!std::isfinite(scale) || scale <= 0.0f || scale > 1.0f)
		{
			throw std::invalid_argument("SliderBar scale must be in (0, 1]");
		}
		if (_scale == scale)
		{
			return;
		}
		_scale = scale;
		_updateBodyGeometry();
	}

	void SliderBar::setRange(float minimum, float maximum)
	{
		setRange(Range{minimum, maximum});
	}

	void SliderBar::setRange(const Range &range)
	{
		if (!std::isfinite(range.minimum) || !std::isfinite(range.maximum) || range.maximum < range.minimum)
		{
			throw std::invalid_argument("SliderBar range must contain finite ascending values");
		}
		_range = range;
	}

	void SliderBar::setRatio(float ratio)
	{
		if (!std::isfinite(ratio))
		{
			throw std::invalid_argument("SliderBar ratio must be finite");
		}
		const float clampedRatio = std::clamp(ratio, 0.0f, 1.0f);
		if (_ratio == clampedRatio)
		{
			return;
		}
		_ratio = clampedRatio;
		_updateBodyGeometry();
		_editionProvider.trigger(_ratio);
	}

	void SliderBar::setValue(float value)
	{
		if (!std::isfinite(value))
		{
			throw std::invalid_argument("SliderBar value must be finite");
		}
		const float extent = _range.maximum - _range.minimum;
		setRatio(extent == 0.0f ? 0.0f : (value - _range.minimum) / extent);
	}

	Orientation SliderBar::orientation() const noexcept
	{
		return _orientation;
	}

	float SliderBar::scale() const noexcept
	{
		return _scale;
	}

	float SliderBar::ratio() const noexcept
	{
		return _ratio;
	}

	float SliderBar::value() const noexcept
	{
		return _range.minimum + (_range.maximum - _range.minimum) * _ratio;
	}

	const SliderBar::Range &SliderBar::range() const noexcept
	{
		return _range;
	}

	bool SliderBar::isDragging() const noexcept
	{
		return _dragging;
	}

	Panel &SliderBar::background() noexcept
	{
		return _background;
	}

	const Panel &SliderBar::background() const noexcept
	{
		return _background;
	}

	Panel &SliderBar::body() noexcept
	{
		return _body;
	}

	const Panel &SliderBar::body() const noexcept
	{
		return _body;
	}
}
