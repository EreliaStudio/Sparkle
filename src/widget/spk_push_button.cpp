#include "widget/spk_push_button.hpp"

#include "structure/graphics/spk_viewport.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
    PushButton::PushButton(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		Widget(p_name, p_parent),
		_isPressed(false),
		_pressedOffset(10, 10),
		_cornerSize(10, 10)
    {

    }

	ContractProvider::Contract PushButton::subscribe(const ContractProvider::Job& p_job)
	{
		return (_onClickProvider.subscribe(p_job));
	}

    void PushButton::setReleasedSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet)
    {
        if (p_spriteSheet == nullptr)
            throw std::invalid_argument("Released SpriteSheet cannot be null.");

        _releasedRenderer.setSpriteSheet(p_spriteSheet);
    }

    void PushButton::setPressedSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet)
    {
        if (p_spriteSheet == nullptr)
		{
            throw std::invalid_argument("Pressed SpriteSheet cannot be null.");
		}

        _pressedRenderer.setSpriteSheet(p_spriteSheet);
    }

	void PushButton::setCornerSize(const spk::Vector2Int& p_cornerSize)
	{
		_cornerSize = p_cornerSize;
		requireGeometryUpdate();
	}

    void PushButton::setPressedOffset(const spk::Vector2Int& p_offset)
    {
        _pressedOffset = p_offset;
		requireGeometryUpdate();
    }

    const spk::Vector2Int& PushButton::pressedOffset() const
    {
        return _pressedOffset;
    }

    void PushButton::_onMouseEvent(spk::MouseEvent& p_event)
    {
		switch (p_event.type)
		{
			case MouseEvent::Type::Motion:
			{
				if (_isPressed == true && geometry().contains(p_event.mouse->position) == false)
				{
					_isPressed = false;
				}
				break;
			}
			case MouseEvent::Type::Press:
			{
				if (p_event.button == spk::Mouse::Button::Left)
				{
					if (_isPressed == false && geometry().contains(p_event.mouse->position) == true)
					{
						_isPressed = true;
					}
				}
				break;
			}
			case MouseEvent::Type::Release:
			{
				if (p_event.button == spk::Mouse::Button::Left)
				{
					if (_isPressed)
					{				
						_onClickProvider.trigger();
						_isPressed = false;
					}
				}
				break;
			}
			default:
			{
				break;
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
        _releasedRenderer.clear();
        _releasedRenderer.prepare(geometry(), layer(), _cornerSize);
        _releasedRenderer.validate();

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
