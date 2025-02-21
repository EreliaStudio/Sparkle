#include "widget/spk_push_button.hpp"

#include "structure/graphics/spk_viewport.hpp"
#include "spk_debug_macro.hpp"

#include "spk_generated_resources.hpp"

namespace spk
{

	spk::SpriteSheet PushButton::_defaultHoverNineSlice = spk::SpriteSheet::fromRawData(
		SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Light.png"),
		spk::Vector2UInt(3, 3),
		SpriteSheet::Filtering::Linear
	);

	spk::SafePointer<spk::SpriteSheet> PushButton::defaultHoverNineSlice()
	{
		return (&_defaultHoverNineSlice);
	}

	PushButton::PushButton(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		Widget(p_name, p_parent),
		_isPressed(false),
		_isHovered(false),
		_pressedOffset(2, 2)
	{
		_cornerSize.released = {10, 10};
		_cornerSize.hovered  = {10, 10};
		_cornerSize.pressed  = {10, 10};

		_text.released = L"";
		_text.hovered  = L"";
		_text.pressed  = L"";

		_verticalAlignment.released   = spk::VerticalAlignment::Centered;
		_verticalAlignment.hovered    = spk::VerticalAlignment::Centered;
		_verticalAlignment.pressed    = spk::VerticalAlignment::Centered;

		_horizontalAlignment.released = spk::HorizontalAlignment::Centered;
		_horizontalAlignment.hovered  = spk::HorizontalAlignment::Centered;
		_horizontalAlignment.pressed  = spk::HorizontalAlignment::Centered;

		_fontRenderer.released.setFont(Widget::defaultFont());
		_fontRenderer.hovered.setFont(Widget::defaultFont());
		_fontRenderer.pressed.setFont(Widget::defaultFont());

		_fontRenderer.released.setGlyphColor(spk::Color::white);
		_fontRenderer.released.setOutlineColor(spk::Color::black);

		_fontRenderer.hovered.setGlyphColor(spk::Color::white);
		_fontRenderer.hovered.setOutlineColor(spk::Color::black);

		_fontRenderer.pressed.setGlyphColor(spk::Color::white);
		_fontRenderer.pressed.setOutlineColor(spk::Color::black);

		_nineSliceRenderer.released.setSpriteSheet(Widget::defaultNineSlice());
		_nineSliceRenderer.hovered.setSpriteSheet(PushButton::defaultHoverNineSlice());
		_nineSliceRenderer.pressed.setSpriteSheet(Widget::defaultNineSlice());

		requireGeometryUpdate();
	}

	spk::Vector2UInt PushButton::computeTextSize() const
	{
		spk::Vector2UInt maxSize(0, 0);

		for (auto s : {State::Released, State::Hovered, State::Pressed})
		{
			auto size = _fontRenderer[s].font()->computeStringSize(_text[s], _fontRenderer[s].fontSize());
			maxSize = spk::Vector2UInt::max(maxSize, size);
		}
		return maxSize;
	}

	spk::Vector2UInt PushButton::computeExpectedTextSize(const spk::Font::Size& p_textSize) const
	{
		spk::Vector2UInt maxSize(0, 0);

		for (auto s : {State::Released, State::Hovered, State::Pressed})
		{
			auto size = _fontRenderer[s].font()->computeStringSize(_text[s], p_textSize);
			maxSize = spk::Vector2UInt::max(maxSize, size);
		}
		return maxSize;
	}

	spk::Vector2UInt PushButton::minimalSize() const
	{
		return (computeTextSize());
	}

	ContractProvider::Contract PushButton::subscribe(const ContractProvider::Job& p_job)
	{
		return _onClickProvider.subscribe(p_job);
	}

	void PushButton::setFont(const spk::SafePointer<spk::Font>& p_font, const State& p_state)
	{
		_fontRenderer[p_state].setFont(p_font);
		requireGeometryUpdate();
	}

	void PushButton::setText(const std::wstring& p_text, const State& p_state)
	{
		_text[p_state] = p_text;
		requireGeometryUpdate();
	}

	void PushButton::setFontSize(const spk::Font::Size& p_fontSize, const State& p_state)
	{
		_fontRenderer[p_state].setFontSize(p_fontSize);
		requireGeometryUpdate();
	}

	void PushButton::setTextColor(const spk::Color& p_glyphColor, const spk::Color& p_outlineColor, const State& p_state)
	{
		_fontRenderer[p_state].setGlyphColor(p_glyphColor);
		_fontRenderer[p_state].setOutlineColor(p_outlineColor);
		requireGeometryUpdate();
	}

	void PushButton::setTextAlignment(const spk::HorizontalAlignment& p_horizontalAlignment,
	                                  const spk::VerticalAlignment& p_verticalAlignment, 
	                                  const State& p_state)
	{
		_horizontalAlignment[p_state] = p_horizontalAlignment;
		_verticalAlignment[p_state]   = p_verticalAlignment;
		requireGeometryUpdate();
	}

	void PushButton::setIconset(spk::SafePointer<spk::SpriteSheet> p_iconset, const State& p_state)
	{
		_iconRenderer[p_state].setTexture(p_iconset);
		requireGeometryUpdate();
	}

	void PushButton::setIcon(const spk::SpriteSheet::Sprite& p_icon, const State& p_state)
	{
		_icon[p_state] = p_icon;
		requireGeometryUpdate();
	}

	void PushButton::setCornerSize(const spk::Vector2Int& p_cornerSize, const State& p_state)
	{
		_cornerSize[p_state] = p_cornerSize;
		requireGeometryUpdate();
	}

	void PushButton::setNineSlice(const spk::SafePointer<spk::SpriteSheet>& p_spriteSheet, const State& p_state)
	{
		_nineSliceRenderer[p_state].setSpriteSheet(p_spriteSheet);
		requireGeometryUpdate();
	}

	void PushButton::setPressedOffset(const spk::Vector2Int& p_offset)
	{
		_pressedOffset = p_offset;
		requireGeometryUpdate();
	}

	void PushButton::setFont(const spk::SafePointer<spk::Font>& p_font)
	{
		setFont(p_font, State::Released);
		setFont(p_font, State::Hovered);
		setFont(p_font, State::Pressed);
	}

	void PushButton::setText(const std::wstring& p_text)
	{
		setText(p_text, State::Released);
		setText(p_text, State::Hovered);
		setText(p_text, State::Pressed);
	}

	void PushButton::setFontSize(const spk::Font::Size& p_fontSize)
	{
		setFontSize(p_fontSize, State::Released);
		setFontSize(p_fontSize, State::Hovered);
		setFontSize(p_fontSize, State::Pressed);
	}

	void PushButton::setTextColor(const spk::Color& p_glyphColor, const spk::Color& p_outlineColor)
	{
		setTextColor(p_glyphColor, p_outlineColor, State::Released);
		setTextColor(p_glyphColor, p_outlineColor, State::Hovered);
		setTextColor(p_glyphColor, p_outlineColor, State::Pressed);
	}

	void PushButton::setTextAlignment(const spk::HorizontalAlignment& p_horizontalAlignment, 
									const spk::VerticalAlignment& p_verticalAlignment)
	{
		setTextAlignment(p_horizontalAlignment, p_verticalAlignment, State::Released);
		setTextAlignment(p_horizontalAlignment, p_verticalAlignment, State::Hovered);
		setTextAlignment(p_horizontalAlignment, p_verticalAlignment, State::Pressed);
	}

	void PushButton::setIconset(spk::SafePointer<spk::SpriteSheet> p_iconset)
	{
		setIconset(p_iconset, State::Released);
		setIconset(p_iconset, State::Hovered);
		setIconset(p_iconset, State::Pressed);
	}

	void PushButton::setIcon(const spk::SpriteSheet::Sprite& p_icon)
	{
		setIcon(p_icon, State::Released);
		setIcon(p_icon, State::Hovered);
		setIcon(p_icon, State::Pressed);
	}

	void PushButton::setCornerSize(const spk::Vector2Int& p_cornerSize)
	{
		setCornerSize(p_cornerSize, State::Released);
		setCornerSize(p_cornerSize, State::Hovered);
		setCornerSize(p_cornerSize, State::Pressed);
	}

	void PushButton::setNineSlice(const SafePointer<SpriteSheet>& p_spriteSheet)
	{
		setNineSlice(p_spriteSheet, State::Released);
		setNineSlice(p_spriteSheet, State::Hovered);
		setNineSlice(p_spriteSheet, State::Pressed);
	}


	const spk::Vector2Int& PushButton::pressedOffset() const
	{
		return _pressedOffset;
	}

	const spk::SafePointer<spk::SpriteSheet>& PushButton::spriteSheet(State p_state) const
	{
		return safe_pointer_cast<spk::SpriteSheet>(_nineSliceRenderer[p_state].spriteSheet());
	}

	const spk::SafePointer<spk::SpriteSheet>& PushButton::iconset(State p_state) const
	{
		return safe_pointer_cast<spk::SpriteSheet>(_iconRenderer[p_state].texture());
	}

	const spk::SafePointer<spk::Font>& PushButton::font(State p_state) const
	{
		return _fontRenderer[p_state].font();
	}

	const spk::Vector2UInt& PushButton::cornerSize(State p_state) const
	{
		return _cornerSize[p_state];
	}

	const std::wstring& PushButton::text(State p_state) const
	{
		return _text[p_state];
	}

	const spk::Font::Size& PushButton::fontSize(State p_state) const
	{
		return _fontRenderer[p_state].fontSize();
	}

	const spk::SpriteSheet::Sprite& PushButton::icon(State p_state) const
	{
		return _icon[p_state];
	}

	const spk::VerticalAlignment& PushButton::verticalAlignment(State p_state) const
	{
		return _verticalAlignment[p_state];
	}

	const spk::HorizontalAlignment& PushButton::horizontalAlignment(State p_state) const
	{
		return _horizontalAlignment[p_state];
	}

	void PushButton::_onMouseEvent(spk::MouseEvent& p_event)
	{
		bool wasHovered = _isHovered;
		bool wasPressed = _isPressed;
		bool inside     = isPointed(p_event.mouse);

		switch (p_event.type)
		{
			case MouseEvent::Type::Motion:
			{
				_isHovered = inside;
				if (_isPressed && !inside)
					_isPressed = false;

				if (_isHovered != wasHovered || _isPressed != wasPressed)
					requestPaint();
				break;
			}
			case MouseEvent::Type::Press:
			case MouseEvent::Type::DoubleClick:
			{
				if (p_event.button == spk::Mouse::Button::Left)
				{
					_isHovered = inside;
					if (inside && !_isPressed)
					{
						_isPressed = true;
						requestPaint();
						p_event.consume();
					}
				}
				break;
			}
			case MouseEvent::Type::Release:
			{
				if (p_event.button == spk::Mouse::Button::Left && _isPressed)
				{
					if (inside)
					{
						_onClickProvider.trigger();
						p_event.consume();
					}
					_isPressed = false;
					requestPaint();
				}
				break;
			}
			default:
				break;
		}
	}

	void PushButton::_onPaintEvent(spk::PaintEvent& p_event)
	{
		State st = _currentVisualState();

		_nineSliceRenderer[st].render();
		_fontRenderer[st].render();
		_iconRenderer[st].render();
	}

	void PushButton::_onGeometryChange()
	{
		using namespace spk;

		Geometry2D releasedGeom = geometry();
		Geometry2D hoveredGeom  = geometry();
		Geometry2D pressedGeom  = geometry().shrink(_pressedOffset);

		for (auto s : {State::Released, State::Hovered, State::Pressed})
		{
			_nineSliceRenderer[s].clear();
			Geometry2D geomToUse = (s == State::Pressed) ? pressedGeom : geometry(); 
			_nineSliceRenderer[s].prepare(geomToUse, layer(), _cornerSize[s]);
			_nineSliceRenderer[s].validate();
		}

		for (auto s : {State::Released, State::Hovered, State::Pressed})
		{
			_fontRenderer[s].clear();

			Geometry2D geomToUse = (s == State::Pressed) ? pressedGeom : geometry();
			geomToUse = geomToUse.shrink(_cornerSize[s]);

			auto textAnchor = _fontRenderer[s].computeTextAnchor(
				geomToUse, 
				_text[s], 
				_horizontalAlignment[s], 
				_verticalAlignment[s]
			);
			_fontRenderer[s].prepare(_text[s], textAnchor, layer() + 0.01f);
			_fontRenderer[s].validate();
		}

		for (auto s : {State::Released, State::Hovered, State::Pressed})
		{
			_iconRenderer[s].clear();

			Geometry2D geomToUse = (s == State::Pressed) ? pressedGeom : geometry();
			geomToUse = geomToUse.shrink(_cornerSize[s]);

			_iconRenderer[s].prepare(geomToUse, _icon[s], layer() + 0.0001f);
			_iconRenderer[s].validate();
		}
	}
}
