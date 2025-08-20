#include "widget/spk_text_edit.hpp"

namespace spk
{
	void TextEdit::_computeCursorsValues()
	{
		const std::wstring &textToRender = renderedText();

		if (_needLowerCursorUpdate == true)
		{
			_lowerCursor = _cursor;

			while (_lowerCursor != 0 && _fontRenderer.computeTextSize(textToRender.substr(_lowerCursor - 1, _cursor - _lowerCursor + 1)).x <=
											geometry().shrink(_cornerSize).size.x)
			{
				_lowerCursor--;
			}

			_needLowerCursorUpdate = false;
		}

		if (_needHigherCursorUpdate == true)
		{
			_higherCursor = _cursor;

			while (_higherCursor <= textToRender.size() &&
				   _fontRenderer.computeTextSize(textToRender.substr(_lowerCursor, _higherCursor - _lowerCursor + 1)).x <
					   geometry().shrink(_cornerSize).size.x)
			{
				_higherCursor++;
			}

			_needHigherCursorUpdate = false;
		}
	}

	void TextEdit::_onGeometryChange()
	{
		if (_needLowerCursorUpdate == true || _needHigherCursorUpdate == true)
		{
			_computeCursorsValues();
		}

		spk::Geometry2D originGeometry = geometry().atOrigin();

		_backgroundRenderer.clear();
		_backgroundRenderer.prepare(originGeometry, layer(), _cornerSize);
		_backgroundRenderer.validate();

		std::wstring tmpText = renderedText();

		_fontRenderer.clear();
		spk::Vector2Int textAnchor = _fontRenderer.computeTextAnchor(
			originGeometry.shrink(_cornerSize), tmpText.substr(_lowerCursor, _higherCursor - _lowerCursor), _horizontalAlignment, _verticalAlignment);
		_fontRenderer.prepare(tmpText.substr(_lowerCursor, _higherCursor - _lowerCursor), textAnchor, layer() + 0.01f);
		_fontRenderer.validate();

		_cursorRenderer.clear();
		spk::Vector2UInt prevTextSize = _fontRenderer.computeTextSize(tmpText.substr(_lowerCursor, _cursor - _lowerCursor));
		_cursorRenderer.prepareSquare(
			spk::Geometry2D(prevTextSize.x + _cornerSize.x - 2, _cornerSize.y, 2, geometry().height - _cornerSize.y * 2), layer() + 0.02f);
		_cursorRenderer.validate();
	}

	void TextEdit::_onPaintEvent(spk::PaintEvent &p_event)
	{
		_backgroundRenderer.render();
		_fontRenderer.render();

		if (hasFocus(Widget::FocusType::KeyboardFocus) == true && _renderCursor == true)
		{
			_cursorRenderer.render();
		}
	}

	void TextEdit::_onUpdateEvent(spk::UpdateEvent &p_event)
	{
		if (p_event.deltaTime != spk::Duration(0ll, spk::TimeUnit::Millisecond))
		{
			bool lastRenderCursor = _renderCursor;

			_renderCursor = (p_event.time.milliseconds / 250) % 2 == 0;

			if (lastRenderCursor != _renderCursor)
			{
				requestPaint();
			}
		}
	}

	void TextEdit::_onMouseEvent(spk::MouseEvent &p_event)
	{
		if (_isEditEnable == false)
		{
			if (focusedWidget(Widget::FocusType::KeyboardFocus) == this)
			{
				releaseFocus(Widget::FocusType::KeyboardFocus);
			}
			return;
		}

		if (p_event.type == spk::MouseEvent::Type::Press)
		{
			if (viewport().geometry().contains(p_event.mouse->position()) == true)
			{
				takeFocus(Widget::FocusType::KeyboardFocus);
			}
			else if (focusedWidget(Widget::FocusType::KeyboardFocus) == this)
			{
				releaseFocus(Widget::FocusType::KeyboardFocus);
			}
			requestPaint();
		}
	}

	void TextEdit::_onKeyboardEvent(spk::KeyboardEvent &p_event)
	{
		if (_isEditEnable == false)
		{
			if (focusedWidget(Widget::FocusType::KeyboardFocus) == this)
			{
				releaseFocus(Widget::FocusType::KeyboardFocus);
			}
			return;
		}

		if (hasFocus(Widget::FocusType::KeyboardFocus) == false)
		{
			return;
		}

		switch (p_event.type)
		{
		case spk::KeyboardEvent::Type::Press:
		case spk::KeyboardEvent::Type::Repeat:
			_handleKeyPress(p_event.key);
			break;

		case spk::KeyboardEvent::Type::Glyph:
			_handleGlyph(p_event.glyph);
			break;

		default:
			break;
		}
	}

	void TextEdit::_handleKeyPress(spk::Keyboard::Key p_key)
	{
		switch (p_key)
		{
		case spk::Keyboard::LeftArrow:
			_moveCursorLeft();
			break;
		case spk::Keyboard::RightArrow:
			_moveCursorRight();
			break;
		case spk::Keyboard::Escape:
			releaseFocus(Widget::FocusType::KeyboardFocus);
			break;
		case spk::Keyboard::Delete:
			_deleteAtCursor();
			break;
		default:
			break;
		}
	}

	void TextEdit::_handleGlyph(wchar_t p_glyph)
	{
		if (p_glyph == spk::Keyboard::Backspace)
		{
			_backspace();
		}
		else if (p_glyph >= 32)
		{
			_insertGlyph(p_glyph);
		}
	}

	void TextEdit::_moveCursorLeft()
	{
		if (_cursor == 0)
		{
			return;
		}

		_cursor--;
		if (_cursor < _lowerCursor)
		{
			_lowerCursor = _cursor;
			_needHigherCursorUpdate = true;
		}
		requireGeometryUpdate();
	}

	void TextEdit::_moveCursorRight()
	{
		if (_cursor >= _text.size())
		{
			return;
		}

		_cursor++;
		if (_cursor >= _higherCursor)
		{
			_higherCursor = _cursor;
			_needLowerCursorUpdate = true;
		}
		requireGeometryUpdate();
	}

	void TextEdit::_deleteAtCursor()
	{
		if (_cursor >= _text.size())
		{
			return;
		}

		_text.erase(_cursor, 1);
		_needHigherCursorUpdate = true;
		requireGeometryUpdate();
	}

	void TextEdit::_backspace()
	{
		if (_text.empty() || _cursor == 0)
		{
			return;
		}

		_text.erase(_cursor - 1, 1);
		_cursor--;

		if (_cursor <= _lowerCursor)
		{
			_lowerCursor = (_lowerCursor > 0) ? _cursor - 1 : 0;
			_needHigherCursorUpdate = true;
		}
		requireGeometryUpdate();
	}

	void TextEdit::_insertGlyph(wchar_t p_glyph)
	{
		_text.insert(_cursor, 1, p_glyph);
		_cursor++;

		if (_cursor > _higherCursor)
		{
			_higherCursor = _cursor;
			_needHigherCursorUpdate = true;
			_needLowerCursorUpdate = true;
		}
		requireGeometryUpdate();
	}

	TextEdit::TextEdit(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_text(L""),
		_placeholder(L"Enter text here")
	{
		_cursorRenderer.setColor(spk::Color(0, 0, 0, 150));

		_onFontResizeContract = _fontRenderer.subscribeToFontEdition([&]() { requireGeometryUpdate(); });

		setFontColor(spk::Color::white, spk::Color::black);
		setTextAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
		setNineSlice(Widget::defaultNineSlice());
		setFont(Widget::defaultFont());
	}

	spk::Vector2UInt TextEdit::minimalSize() const
	{
		return (_fontRenderer.computeTextSize(_placeholder) + _cornerSize * 2);
	}

	void TextEdit::setObscured(bool p_state)
	{
		_isObscured = p_state;
		requireGeometryUpdate();
	}

	void TextEdit::setNineSlice(const spk::SafePointer<const spk::SpriteSheet> &p_spriteSheet)
	{
		_backgroundRenderer.setSpriteSheet(p_spriteSheet);
	}

	void TextEdit::setCornerSize(const spk::Vector2Int &p_cornerSize)
	{
		_cornerSize = p_cornerSize;
		requireGeometryUpdate();
	}

	void TextEdit::setFont(const spk::SafePointer<spk::Font> &p_font)
	{
		_fontRenderer.setFont(p_font);

		requireGeometryUpdate();
	}

	void TextEdit::setFontSize(const spk::Font::Size &p_textSize)
	{
		_fontRenderer.setFontSize(p_textSize);
		requireGeometryUpdate();
	}

	void TextEdit::setFontColor(const spk::Color &p_glyphColor, const spk::Color &p_outlineColor)
	{
		_fontRenderer.setGlyphColor(p_glyphColor);
		_fontRenderer.setOutlineColor(p_outlineColor);
	}

	void TextEdit::setTextAlignment(const spk::HorizontalAlignment &p_horizontalAlignment, const spk::VerticalAlignment &p_verticalAlignment)
	{
		_horizontalAlignment = p_horizontalAlignment;
		_verticalAlignment = p_verticalAlignment;
		requireGeometryUpdate();
	}

	void TextEdit::setText(const std::wstring &p_text)
	{
		_text = p_text;
		_cursor = _text.size();
		_needLowerCursorUpdate = true;
		_needHigherCursorUpdate = true;
		requireGeometryUpdate();
	}

	void TextEdit::setPlaceholder(const std::wstring &p_placeholder)
	{
		_placeholder = p_placeholder;
	}

	bool TextEdit::hasText() const
	{
		return (_text.empty() == false);
	}

	bool TextEdit::isObscured() const
	{
		return (_isObscured);
	}

	bool TextEdit::isEditEnable() const
	{
		return (_isEditEnable);
	}

	void TextEdit::disableEdit()
	{
		_isEditEnable = false;
	}

	void TextEdit::enableEdit()
	{
		_isEditEnable = true;
	}

	std::wstring TextEdit::renderedText() const
	{
		if (hasText() == true)
		{
			if (_isObscured == true)
			{
				return (std::wstring(_text.size(), '*'));
			}
			return (_text);
		}
		return (_placeholder);
	}

	const std::wstring &TextEdit::text() const
	{
		if (hasText() == true)
		{
			return (_text);
		}
		return (_placeholder);
	}

	const spk::Vector2Int &TextEdit::cornerSize() const
	{
		return (_cornerSize);
	}
}