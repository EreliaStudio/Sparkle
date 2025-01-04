#include "widget/spk_push_button.hpp"

#include "structure/graphics/spk_viewport.hpp"

namespace spk
{
    PushButton::PushButton(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		Widget(p_name, p_parent),
		_isPressed(false),
		_pressedOffset(10, 10),
		_cornerSize(10, 10)
    {

    }

    void PushButton::setReleasedSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet)
    {
        if (!p_spriteSheet)
            throw std::invalid_argument("Released SpriteSheet cannot be null.");

        _releasedSpriteSheet = p_spriteSheet;
        _releasedRenderer.setSpriteSheet(_releasedSpriteSheet);

        _releasedRenderer.prepare(geometry(), layer(), _cornerSize);
        _releasedRenderer.validate();
    }

    void PushButton::setPressedSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet)
    {
        if (!p_spriteSheet)
            throw std::invalid_argument("Pressed SpriteSheet cannot be null.");

        _pressedSpriteSheet = p_spriteSheet;
        _pressedRenderer.setSpriteSheet(_pressedSpriteSheet);

        Geometry2D pressedGeometry = geometry();
        pressedGeometry.x += _pressedOffset.x;
        pressedGeometry.y += _pressedOffset.y;
        pressedGeometry.width -= 2 * _pressedOffset.x;
        pressedGeometry.height -= 2 * _pressedOffset.y;

        _pressedRenderer.prepare(pressedGeometry, layer(), _cornerSize);
        _pressedRenderer.validate();
    }

    void PushButton::_prepareRenderers()
    {
        if (_releasedSpriteSheet)
        {
            _releasedRenderer.clear();
            _releasedRenderer.prepare(geometry(), layer(), _cornerSize);
            _releasedRenderer.validate();
        }

        if (_pressedSpriteSheet)
        {
            Geometry2D pressedGeometry = geometry();
            pressedGeometry.x += _pressedOffset.x;
            pressedGeometry.y += _pressedOffset.y;
            pressedGeometry.width -= 2 * _pressedOffset.x;
            pressedGeometry.height -= 2 * _pressedOffset.y;

            _pressedRenderer.clear();
            _pressedRenderer.prepare(pressedGeometry, layer(), _cornerSize);
            _pressedRenderer.validate();
        }
    }

    void PushButton::setPressedOffset(const spk::Vector2Int& p_offset)
    {
        _pressedOffset = p_offset;
        _prepareRenderers();
    }

    const spk::Vector2Int& PushButton::pressedOffset() const
    {
        return _pressedOffset;
    }

    void PushButton::_onMouseEvent(spk::MouseEvent& p_event)
    {
		if (p_event.type == MouseEvent::Type::Motion)
		{
			if (_isPressed == true && geometry().contains(p_event.mouse->position) == false)
            {
                _isPressed = true;
            }
		}
        else if (p_event.type == MouseEvent::Type::Press)
        {
			if (p_event.button == spk::Mouse::Button::Left)
			{
				if (_isPressed == false && geometry().contains(p_event.mouse->position) == true)
				{
					_isPressed = true;
				}
			}
        }
        else if (p_event.type == MouseEvent::Type::Release)
        {
            if (p_event.button == spk::Mouse::Button::Left)
			{
				if (_isPressed)
            	{
            	    _isPressed = false;
            	}
			}
        }
    }

    void PushButton::_onPaintEvent(spk::PaintEvent& p_event)
    {
        if (_isPressed == true)
        {
            _pressedRenderer.render();
        }
        else
        {
            _releasedRenderer.render();
        }
    }

    void PushButton::_onGeometryChange()
    {
        _prepareRenderers();
    }
}
