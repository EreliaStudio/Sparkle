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

	void PushButton::setTextAlignment(const spk::HorizontalAlignment& p_horizontalAlignment, const spk::VerticalAlignment& p_verticalAlignment)
	{
		_releasedHorizontalAlignment = p_horizontalAlignment;
		_releasedVerticalAlignment = p_verticalAlignment;
		_pressedHorizontalAlignment = p_horizontalAlignment;
		_pressedVerticalAlignment = p_verticalAlignment;
		requireGeometryUpdate();
	}
	
	void PushButton::setTextAlignment(
		const spk::HorizontalAlignment& p_releasedHorizontalAlignment, const spk::VerticalAlignment& p_releasedVerticalAlignment, 
		const spk::HorizontalAlignment& p_pressedHorizontalAlignment, const spk::VerticalAlignment& p_pressedVerticalAlignment
	)
	{
		_releasedHorizontalAlignment = p_releasedHorizontalAlignment;
		_releasedVerticalAlignment = p_releasedVerticalAlignment;
		_pressedHorizontalAlignment = p_pressedHorizontalAlignment;
		_pressedVerticalAlignment = p_pressedVerticalAlignment;
		requireGeometryUpdate();
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
            _pressedRenderer.render();
            _pressedFontRenderer.render();
        }
        else
        {
            _releasedRenderer.render();
            _releasedFontRenderer.render();
        }
    }

    void PushButton::_onGeometryChange()
    {
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

		_releasedFontRenderer.clear();
		spk::Vector2Int textAnchor = geometry().anchor + _releasedFontRenderer.computeTextAnchor(_releasedText, _releasedHorizontalAlignment, _releasedVerticalAlignment);
		switch (_releasedHorizontalAlignment)
		{
			case spk::HorizontalAlignment::Left:
			{
				textAnchor.x += 0;
				break;
			}
			case spk::HorizontalAlignment::Centered:
			{
				textAnchor.x += geometry().size.x / 2;
				break;
			}
			case spk::HorizontalAlignment::Right:
			{
				textAnchor.x += geometry().size.x;
				break;
			}
		}
		switch (_releasedVerticalAlignment)
		{
			case spk::VerticalAlignment::Top:
			{
				textAnchor.y += 0;
				break;
			}
			case spk::VerticalAlignment::Centered:
			{
				textAnchor.y += geometry().size.y / 2;
				break;
			}
			case spk::VerticalAlignment::Down:
			{
				textAnchor.y += geometry().size.y;
				break;
			}
		}
		_releasedFontRenderer.prepare(_releasedText, textAnchor, layer() + 0.01f);
		_releasedFontRenderer.validate();

		textAnchor = geometry().anchor + _pressedFontRenderer.computeTextAnchor(_pressedText, _pressedHorizontalAlignment, _pressedVerticalAlignment);
		switch (_pressedHorizontalAlignment)
		{
			case spk::HorizontalAlignment::Left:
			{
				textAnchor.x += 0;
				break;
			}
			case spk::HorizontalAlignment::Centered:
			{
				textAnchor.x += geometry().size.x / 2;
				break;
			}
			case spk::HorizontalAlignment::Right:
			{
				textAnchor.x += geometry().size.x;
				break;
			}
		}
		switch (_pressedVerticalAlignment)
		{
			case spk::VerticalAlignment::Top:
			{
				textAnchor.y += 0;
				break;
			}
			case spk::VerticalAlignment::Centered:
			{
				textAnchor.y += geometry().size.y / 2;
				break;
			}
			case spk::VerticalAlignment::Down:
			{
				textAnchor.y += geometry().size.y;
				break;
			}
		}
		_pressedFontRenderer.clear();
		_pressedFontRenderer.prepare(_pressedText, textAnchor, layer() + 0.01f);
		_pressedFontRenderer.validate();
    }
}
