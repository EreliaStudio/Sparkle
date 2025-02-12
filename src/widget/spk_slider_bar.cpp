#include "widget/spk_slider_bar.hpp"

#include "spk_generated_resources.hpp"

namespace spk
{
	spk::SpriteSheet SliderBar::_defaultSliderBody = 
		spk::SpriteSheet::fromRawData(
			SPARKLE_GET_RESOURCE("resources/textures/defaultSliderBody.png"),
			spk::Vector2Int(3, 3),
			spk::SpriteSheet::Filtering::Linear
		);

	void SliderBar::_onGeometryChange()
	{
		_backgroundRenderer.clear();
		_backgroundRenderer.prepare(geometry(), layer(), _cornerSize);
		_backgroundRenderer.validate();

		spk::Vector2UInt bodySize;
		spk::Vector2UInt bodyOffset;

		switch (_orientation)
		{
			case Orientation::Horizontal:
			{
				bodySize = spk::Vector2UInt(geometry().size.x * _scale, geometry().size.y);
				bodyOffset = spk::Vector2UInt((geometry().size.x * (1 - _scale)), 0);
				break;
			}
			case Orientation::Vertical:
			{
				bodySize = spk::Vector2UInt(geometry().size.x, geometry().size.y * _scale);
				bodyOffset = spk::Vector2UInt(0, (geometry().size.y * (1 - _scale)));
				break;
			}
		}

		_body.setGeometry(bodyOffset * _ratio, bodySize);
	}

	void SliderBar::_onPaintEvent(spk::PaintEvent& p_event)
	{
		_backgroundRenderer.render();
	}

	void SliderBar::_onMouseEvent(spk::MouseEvent& p_event)
	{
		switch (p_event.type)
		{
			case spk::MouseEvent::Type::Press:
			{
				if (p_event.button == spk::Mouse::Button::Left)
				{
					if (_body.viewport().geometry().contains(p_event.mouse->position) == true)
					{
						_isClicked = true;
						_clickedMousePosition = p_event.mouse->position;
						_clickedRatio = _ratio;
					}
				}
					
				break;
			}
			case spk::MouseEvent::Type::Release:
			{
				if (p_event.button == spk::Mouse::Button::Left)
				{
					_isClicked = false;
				}
				break;
			}
			case spk::MouseEvent::Type::Motion:
			{
				if (_isClicked == true && p_event.mouse->deltaPosition != 0)
				{
					float range = (_orientation == Orientation::Horizontal ? geometry().size.x * (1 - _scale) : geometry().size.y * (1 - _scale));
					float mouseDeltaPosition = (_orientation == Orientation::Horizontal ?
						static_cast<int>(p_event.mouse->position.x) - static_cast<int>(_clickedMousePosition.x) :
						static_cast<int>(p_event.mouse->position.y) - static_cast<int>(_clickedMousePosition.y));

					float delta = mouseDeltaPosition / range;

					_ratio = std::clamp(_clickedRatio + delta, 0.0f, 1.0f);

					_onEditionContractProvider.trigger(_ratio);
					requireGeometryUpdate();
				}
			}
		}
	}

	SliderBar::SliderBar(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_cornerSize(2, 2),
		_scale(0.1f),
		_body(p_name + L" - Body", this)
	{
		_backgroundRenderer.setSpriteSheet(spk::Widget::defaultNineSlice());

		_body.setSpriteSheet(&_defaultSliderBody);
		_body.activate();
	}

	SliderBar::Contract SliderBar::subscribe(const Job& p_job)
	{
		return (std::move(_onEditionContractProvider.subscribe(p_job)));
	}

	void SliderBar::setOrientation(const SliderBar::Orientation& p_orientation)
	{
		_orientation = p_orientation;
		requireGeometryUpdate();
	}

	void SliderBar::setCornerSize(const spk::Vector2UInt& p_cornerSize)
	{
		_cornerSize = p_cornerSize;
	}

	void SliderBar::setBodyCornerSize(const spk::Vector2UInt& p_bodyCornerSize)
	{
		_body.setCornerSize(p_bodyCornerSize);
	}

	void SliderBar::setSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet)
	{
		_backgroundRenderer.setSpriteSheet(p_spriteSheet);
	}

	void SliderBar::setBodySpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet)
	{
		_body.setSpriteSheet(p_spriteSheet);
	}

	void SliderBar::setScale(const float& p_scale)
	{
		_scale = p_scale;
	}

	void SliderBar::setRange(float p_minValue, float p_maxValue)
	{
		_minValue = p_minValue;
		_maxValue = p_maxValue;
	}

	float SliderBar::value()
	{
		return (std::lerp(_minValue, _maxValue, _ratio));
	}

	float SliderBar::ratio()
	{
		return (_ratio);
	}

	const SliderBar::Orientation& SliderBar::orientation() const
	{
		return (_orientation);
	}
}