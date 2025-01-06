#include "widget/spk_push_button.hpp"

#include "structure/graphics/spk_viewport.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
    PushButton::PushButton(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		Widget(p_name, p_parent),
		_isPressed(false),
		_pressedOffset(10, 10),
		_releasedCornerSize(10, 10),
		_pressedCornerSize(10, 10)
    {

    }

	ContractProvider::Contract PushButton::subscribe(const ContractProvider::Job& p_job)
	{
		return (_onClickProvider.subscribe(p_job));
	}

	void PushButton::setFont(const spk::SafePointer<spk::Font>& p_font)
	{
		_releasedFontRenderer.setFont(p_font);
		_pressedFontRenderer.setFont(p_font);
		requireGeometryUpdate();
	}
	
	void PushButton::setFont(const spk::SafePointer<spk::Font>& p_releasedFont, const spk::SafePointer<spk::Font>& p_pressedFont)
	{
		_releasedFontRenderer.setFont(p_releasedFont);
		_pressedFontRenderer.setFont(p_pressedFont);
		requireGeometryUpdate();
	}

	void PushButton::setText(const std::wstring& p_text)
	{
		_releasedText = p_text;
		_pressedText = p_text;
		requireGeometryUpdate();
	}
	
	void PushButton::setText(const std::wstring& p_releasedText, const std::wstring& p_pressedText)
	{
		_releasedText = p_releasedText;
		_pressedText = p_pressedText;
		requireGeometryUpdate();
	}

	void PushButton::setTextSize(const spk::Font::Size& p_textSize)
	{
		_releasedFontRenderer.setFontSize(p_textSize);
		_pressedFontRenderer.setFontSize(p_textSize);
		requireGeometryUpdate();
	}
	
	void PushButton::setTextSize(const spk::Font::Size& p_releasedTextSize, const spk::Font::Size& p_pressedTextSize)
	{
		_releasedFontRenderer.setFontSize(p_releasedTextSize);
		_pressedFontRenderer.setFontSize(p_pressedTextSize);
		requireGeometryUpdate();
	}

	void PushButton::setTextColor(const spk::Color& p_glyphColor, const spk::Color& p_outlineColor)
	{
		_releasedFontRenderer.setGlyphColor(p_glyphColor);
		_releasedFontRenderer.setOutlineColor(p_outlineColor);
		
		_pressedFontRenderer.setGlyphColor(p_glyphColor);
		_pressedFontRenderer.setOutlineColor(p_outlineColor);
	}

	void PushButton::setTextColor(
			const spk::Color& p_releasedGlyphColor, const spk::Color& p_releasedOutlineColor,
			const spk::Color& p_pressedGlyphColor, const spk::Color& p_pressedOutlineColor
		)
	{
		_releasedFontRenderer.setGlyphColor(p_releasedGlyphColor);
		_releasedFontRenderer.setOutlineColor(p_releasedOutlineColor);
		
		_pressedFontRenderer.setGlyphColor(p_pressedGlyphColor);
		_pressedFontRenderer.setOutlineColor(p_pressedOutlineColor);
	}

	void PushButton::setCornerSize(const spk::Vector2Int& p_cornerSize)
	{
		_releasedCornerSize = p_cornerSize;
		_pressedCornerSize = p_cornerSize;
		requireGeometryUpdate();
	}

	void PushButton::setCornerSize(const spk::Vector2Int& p_releasedCornerSize, const spk::Vector2Int& p_pressedCornerSize)
	{
		_releasedCornerSize = p_releasedCornerSize;
		_pressedCornerSize = p_pressedCornerSize;
		requireGeometryUpdate();
	}

	void PushButton::setSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument("PushButton spriteSheet cannot be null.");
		}

		_releasedRenderer.setSpriteSheet(p_spriteSheet);
		_pressedRenderer.setSpriteSheet(p_spriteSheet);
	}

	void PushButton::setSpriteSheet(const SafePointer<SpriteSheet>& p_releasedSpriteSheet, const SafePointer<SpriteSheet>& p_pressedSpriteSheet)
	{
		if (p_releasedSpriteSheet == nullptr)
		{
			throw std::invalid_argument("Released SpriteSheet cannot be null.");
		}
		
		if (p_pressedSpriteSheet == nullptr)
		{
			throw std::invalid_argument("Pressed SpriteSheet cannot be null.");
		}

		_releasedRenderer.setSpriteSheet(p_releasedSpriteSheet);
		_pressedRenderer.setSpriteSheet(p_pressedSpriteSheet);
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
            _pressedFontRenderer.render();
            _pressedRenderer.render();
        }
        else
        {
            _releasedFontRenderer.render();
            _releasedRenderer.render();
        }
    }

    void PushButton::_onGeometryChange()
    {
		spk::Vector2Int textSize = _releasedFontRenderer.computeTextSize(_releasedText);
		spk::cout << "Text size : " << textSize << std::endl;
		_releasedFontRenderer.clear();
		_releasedFontRenderer.prepare(_releasedText, geometry().anchor + geometry().size / 2 - spk::Vector2(textSize.x / 2, 0), layer() + 0.01f);
		_releasedFontRenderer.validate();

		textSize = _pressedFontRenderer.computeTextSize(_pressedText);
		_pressedFontRenderer.clear();
		_pressedFontRenderer.prepare(_pressedText, geometry().anchor + geometry().size / 2 - spk::Vector2(textSize.x / 2, 0), layer() + 0.01f);
		_pressedFontRenderer.validate();

        _releasedRenderer.clear();
        _releasedRenderer.prepare(geometry(), layer(), _releasedCornerSize);
        _releasedRenderer.validate();

        Geometry2D pressedGeometry = geometry();
		pressedGeometry.x += _pressedOffset.x;
		pressedGeometry.y += _pressedOffset.y;
		pressedGeometry.width -= 2 * _pressedOffset.x;
		pressedGeometry.height -= 2 * _pressedOffset.y;

		_pressedRenderer.clear();
		_pressedRenderer.prepare(pressedGeometry, layer(), _pressedCornerSize);
		_pressedRenderer.validate();
    }
}
