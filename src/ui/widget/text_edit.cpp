#include "ui/widget/text_edit.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "core/context/update_context.hpp"
#include "graphics/mesh/color_mesh_2d.hpp"
#include "rendering/command/draw_color_mesh_render_command.hpp"
#include "rendering/command/nine_slice_render_command.hpp"
#include "rendering/command/text_render_command.hpp"

namespace
{
	std::string toUTF8(const spk::Font::Text &text)
	{
		std::string result;
		for (char32_t codepoint : text)
		{
			if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
			{
				codepoint = 0xFFFD;
			}
			if (codepoint <= 0x7F)
			{
				result.push_back(static_cast<char>(codepoint));
			}
			else if (codepoint <= 0x7FF)
			{
				result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
				result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
			}
			else if (codepoint <= 0xFFFF)
			{
				result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
				result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
			}
			else
			{
				result.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
				result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
			}
		}
		return result;
	}
}

namespace spk
{
	TextEdit::TextEdit(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		_updateSizeHint();
		activate();
	}

	TextEdit::TextEdit(std::string name, Font *font, Widget *parent) :
		TextEdit(std::move(name), parent)
	{
		setFont(font);
	}

	TextEdit::TextEdit(std::string name, const SpriteSheet *spriteSheet, Font *font, Widget *parent) :
		TextEdit(std::move(name), font, parent)
	{
		setSpriteSheet(spriteSheet);
	}

	Vector2UInt TextEdit::_innerSize() const noexcept
	{
		const unsigned int horizontalCorner = std::min(static_cast<unsigned int>(std::max(_cornerSize.x, 0)), geometry().width / 2);
		const unsigned int verticalCorner = std::min(static_cast<unsigned int>(std::max(_cornerSize.y, 0)), geometry().height / 2);
		return {geometry().width - 2 * horizontalCorner, geometry().height - 2 * verticalCorner};
	}

	Font::Text TextEdit::_editableRepresentation() const
	{
		return _obscured ? Font::Text(_text.size(), U'*') : _text;
	}

	unsigned int TextEdit::_measure(const Font::Text &text) const
	{
		return _font == nullptr || text.empty() ? 0 : _font->computeStringSize(text, _textSize).x;
	}

	Font::Text TextEdit::_fittingPrefix(const Font::Text &text, unsigned int width) const
	{
		Font::Text result;
		for (const char32_t glyph : text)
		{
			Font::Text candidate = result;
			candidate.push_back(glyph);
			if (_measure(candidate) > width)
			{
				break;
			}
			result = std::move(candidate);
		}
		return result;
	}

	TextEdit::ValidationState TextEdit::_validate(const Font::Text &candidate) const
	{
		return _validationCallback ? _validationCallback(candidate) : ValidationState::Valid;
	}

	void TextEdit::_resetCaretBlink() noexcept
	{
		_caretVisible = true;
		_caretElapsed = {};
	}

	void TextEdit::_recomputeVisibleRange()
	{
		_cursor = std::min(_cursor, _text.size());
		if (_text.empty())
		{
			_visibleStart = _visibleEnd = 0;
			return;
		}

		const Font::Text representation = _editableRepresentation();
		const unsigned int innerWidth = _innerSize().x;
		const unsigned int availableWidth = innerWidth > 2 ? innerWidth - 2 : 0;
		_visibleStart = std::min(_visibleStart, _cursor);
		while (_visibleStart < _cursor && _measure(representation.substr(_visibleStart, _cursor - _visibleStart)) > availableWidth)
		{
			++_visibleStart;
		}

		_visibleEnd = _visibleStart;
		while (_visibleEnd < representation.size() && _measure(representation.substr(_visibleStart, _visibleEnd - _visibleStart + 1)) <= availableWidth)
		{
			++_visibleEnd;
		}
		if (_cursor > _visibleEnd)
		{
			_visibleStart = _cursor;
			_visibleEnd = _cursor;
		}
	}

	void TextEdit::_placeCursorFromClick(int globalX)
	{
		if (_text.empty() || _font == nullptr)
		{
			_cursor = 0;
			_recomputeVisibleRange();
			return;
		}

		const Font::Text representation = _editableRepresentation();
		const Font::Text visible = representation.substr(_visibleStart, _visibleEnd - _visibleStart);
		const int horizontalCorner = std::min(_cornerSize.x, static_cast<int>(geometry().width / 2));
		const int localX = globalX - viewRegion().viewport.anchor.x - horizontalCorner;
		if (localX <= 0)
		{
			_cursor = _visibleStart;
		}
		else
		{
			_cursor = _visibleEnd;
			unsigned int previousBoundary = 0;
			for (std::size_t index = 0; index < visible.size(); ++index)
			{
				const unsigned int nextBoundary = _measure(visible.substr(0, index + 1));
				if (localX < static_cast<int>((previousBoundary + nextBoundary) / 2))
				{
					_cursor = _visibleStart + index;
					break;
				}
				previousBoundary = nextBoundary;
			}
		}
		_resetCaretBlink();
		_recomputeVisibleRange();
	}

	void TextEdit::_notifyEdition()
	{
		_editionProvider.trigger(_text);
	}

	void TextEdit::_handleMousePress(EventBase &event, Mouse::Button button, const Vector2Int &position)
	{
		if (button != Mouse::Button::Left)
		{
			return;
		}

		const bool inside = viewRegion().viewport.contains(position);
		if (!_editEnabled)
		{
			if (_focused && !inside)
			{
				_focused = false;
				event.releaseFocus(FocusMode::Channel::Keyboard, this);
			}
			return;
		}

		if (inside)
		{
			_placeCursorFromClick(position.x);
			_focused = true;
			event.takeFocus(FocusMode::Channel::Keyboard, this);
			event.consumed = true;
		}
		else if (_focused)
		{
			_focused = false;
			event.releaseFocus(FocusMode::Channel::Keyboard, this);
		}
	}

	void TextEdit::_updateSizeHint()
	{
		Vector2 intrinsic{
			static_cast<float>(std::max(_cornerSize.x, 0) * 2),
			static_cast<float>(std::max(_cornerSize.y, 0) * 2)};
		if (_font != nullptr && !_placeholder.empty())
		{
			const Vector2UInt placeholderSize = _font->computeStringSize(_placeholder, _textSize);
			intrinsic.x += static_cast<float>(placeholderSize.x);
			intrinsic.y += static_cast<float>(placeholderSize.y);
		}
		SizeHint hint = sizeHint();
		hint.minimal = intrinsic;
		hint.preferred = intrinsic;
		setSizeHint(hint);
	}

	void TextEdit::_updateState(UpdateContext &context)
	{
		if (!_focused || !_editEnabled)
		{
			_caretVisible = false;
			_caretElapsed = {};
			return;
		}
		_caretElapsed += context.deltaTime;
		constexpr auto BlinkDuration = std::chrono::milliseconds(250);
		while (_caretElapsed >= BlinkDuration)
		{
			_caretElapsed -= BlinkDuration;
			_caretVisible = !_caretVisible;
		}
	}

	void TextEdit::_buildRenderSnapshot(RenderSnapshot::Builder &builder)
	{
		if (geometry().width == 0 || geometry().height == 0)
		{
			return;
		}

		auto &pass = builder.renderPass(Widget::OverlayKey);
		const Vector2UInt corner{
			std::min(static_cast<unsigned int>(std::max(_cornerSize.x, 0)), geometry().width / 2),
			std::min(static_cast<unsigned int>(std::max(_cornerSize.y, 0)), geometry().height / 2)};
		if (_spriteSheet != nullptr)
		{
			pass.emplace<NineSliceRenderCommand>(_spriteSheet, Rect2D{Vector2Int{0, 0}, geometry().size}, corner, _depth);
		}

		const Font::Text displayedText = visibleText();
		if (_font != nullptr && !displayedText.empty())
		{
			pass.emplace<TextRenderCommand>(
				_font,
				_textSize,
				displayedText,
				TextRenderCommand::Anchor{
					.position = {static_cast<int>(corner.x), static_cast<int>(corner.y + _innerSize().y / 2)},
					.horizontalAlignment = HorizontalAlignment::Left,
					.verticalAlignment = VerticalAlignment::Center},
				_glyphColor,
				_outlineColor,
				_depth);
		}

		if (_font == nullptr || !_editEnabled || !_focused || !_caretVisible)
		{
			return;
		}
		const Font::Text representation = _editableRepresentation();
		const Font::Text beforeCursor = representation.substr(_visibleStart, _cursor - _visibleStart);
		const unsigned int cursorOffset = _measure(beforeCursor);
		const unsigned int innerWidth = _innerSize().x;
		const unsigned int cursorX = corner.x + std::min(cursorOffset, innerWidth > 2 ? innerWidth - 2 : 0);
		ColorMesh2D::Builder mesh;
		mesh.addShape(
			{{static_cast<float>(cursorX), static_cast<float>(corner.y)}, _depth, _cursorColor},
			{{static_cast<float>(cursorX), static_cast<float>(corner.y + _innerSize().y)}, _depth, _cursorColor},
			{{static_cast<float>(cursorX + 2), static_cast<float>(corner.y + _innerSize().y)}, _depth, _cursorColor},
			{{static_cast<float>(cursorX + 2), static_cast<float>(corner.y)}, _depth, _cursorColor});
		pass.emplace<DrawColorMeshRenderCommand>(std::move(mesh).build());
	}

	void TextEdit::_onGeometryChange()
	{
		_recomputeVisibleRange();
	}

	void TextEdit::_onFocusAcquired(FocusMode::Channel channel) noexcept
	{
		if (channel != FocusMode::Channel::Keyboard || !_editEnabled)
		{
			return;
		}
		_focused = true;
		_resetCaretBlink();
	}

	void TextEdit::_onFocusReleased(FocusMode::Channel channel) noexcept
	{
		if (channel != FocusMode::Channel::Keyboard)
		{
			return;
		}
		_focused = false;
		_caretVisible = false;
		_caretElapsed = {};
	}

	void TextEdit::_onWindowFocusLostEvent(WindowFocusLostEvent &event)
	{
		if (_focused)
		{
			_focused = false;
			event.releaseFocus(FocusMode::Channel::Keyboard, this);
		}
	}

	void TextEdit::_onMouseLeftEvent(MouseLeftEvent &)
	{
		_hovered = false;
	}

	void TextEdit::_onMouseMovedEvent(MouseMovedEvent &event)
	{
		_hovered = _editEnabled && viewRegion().viewport.contains(event.device.position);
	}

	void TextEdit::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		_handleMousePress(event, event.record.button, event.device.position);
	}

	void TextEdit::_onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &event)
	{
		_handleMousePress(event, event.record.button, event.device.position);
	}

	void TextEdit::_onKeyPressedEvent(KeyPressedEvent &event)
	{
		if (!_editEnabled || !_focused)
		{
			return;
		}

		bool recognized = true;
		switch (event.record.key)
		{
		case Keyboard::LeftArrow:
			if (_cursor != 0)
			{
				--_cursor;
			}
			break;
		case Keyboard::RightArrow:
			if (_cursor < _text.size())
			{
				++_cursor;
			}
			break;
		case Keyboard::Delete:
			if (_cursor < _text.size())
			{
				_text.erase(_cursor, 1);
				_notifyEdition();
			}
			break;
		case Keyboard::Backspace:
			if (_cursor != 0)
			{
				_text.erase(--_cursor, 1);
				_notifyEdition();
			}
			break;
		case Keyboard::Escape:
			_focused = false;
			event.releaseFocus(FocusMode::Channel::Keyboard, this);
			break;
		default:
			recognized = false;
			break;
		}
		if (recognized)
		{
			_resetCaretBlink();
			_recomputeVisibleRange();
			event.consumed = true;
		}
	}

	void TextEdit::_onTextInputEvent(TextInputEvent &event)
	{
		if (!_editEnabled || !_focused)
		{
			return;
		}
		event.consumed = true;
		if (event.record.glyph < U' ')
		{
			return;
		}

		Font::Text candidate = _text;
		candidate.insert(candidate.begin() + static_cast<std::ptrdiff_t>(_cursor), event.record.glyph);
		if (_validate(candidate) == ValidationState::Invalid)
		{
			return;
		}
		_text = std::move(candidate);
		++_cursor;
		_resetCaretBlink();
		_recomputeVisibleRange();
		_notifyEdition();
	}

	TextEdit::EditionContract TextEdit::subscribeToEdition(EditionCallback callback)
	{
		return _editionProvider.subscribe(std::move(callback));
	}

	void TextEdit::setSpriteSheet(const SpriteSheet *spriteSheet)
	{
		if (spriteSheet == nullptr)
		{
			throw std::invalid_argument("TextEdit sprite sheet cannot be null");
		}
		if (spriteSheet->nbSprite() != Vector2UInt{3, 3})
		{
			throw std::invalid_argument("TextEdit requires a 3x3 sprite sheet");
		}
		_spriteSheet = spriteSheet;
		_cornerSize = {
			static_cast<int>(spriteSheet->size().x / 3),
			static_cast<int>(spriteSheet->size().y / 3)};
		_updateSizeHint();
		_recomputeVisibleRange();
	}

	void TextEdit::setCornerSize(const Vector2Int &cornerSize)
	{
		if (cornerSize.x < 0 || cornerSize.y < 0)
		{
			throw std::invalid_argument("TextEdit corner size cannot be negative");
		}
		_cornerSize = cornerSize;
		_updateSizeHint();
		_recomputeVisibleRange();
	}

	void TextEdit::setFont(Font *font)
	{
		if (font == nullptr)
		{
			throw std::invalid_argument("TextEdit font cannot be null");
		}
		_font = font;
		_updateSizeHint();
		_recomputeVisibleRange();
	}

	void TextEdit::setText(const Font::Text &text)
	{
		if (_text == text)
		{
			return;
		}
		_text = text;
		_cursor = _text.size();
		_visibleStart = 0;
		_resetCaretBlink();
		_recomputeVisibleRange();
		_notifyEdition();
	}

	void TextEdit::setText(std::string_view text)
	{
		setText(Font::textFromUTF8(text));
	}

	void TextEdit::setPlaceholder(const Font::Text &placeholder)
	{
		if (_placeholder == placeholder)
		{
			return;
		}
		_placeholder = placeholder;
		_updateSizeHint();
	}

	void TextEdit::setPlaceholder(std::string_view placeholder)
	{
		setPlaceholder(Font::textFromUTF8(placeholder));
	}

	void TextEdit::setObscured(bool obscured)
	{
		if (_obscured == obscured)
		{
			return;
		}
		_obscured = obscured;
		_visibleStart = 0;
		_recomputeVisibleRange();
	}

	void TextEdit::enableEdit()
	{
		_editEnabled = true;
	}

	void TextEdit::disableEdit()
	{
		_editEnabled = false;
		_focused = false;
		_hovered = false;
		_caretVisible = false;
	}

	void TextEdit::setValidationCallback(ValidationCallback callback)
	{
		_validationCallback = std::move(callback);
	}

	void TextEdit::setTextSize(const Font::Size &textSize)
	{
		if (_textSize == textSize)
		{
			return;
		}
		_textSize = textSize;
		_updateSizeHint();
		_recomputeVisibleRange();
	}

	void TextEdit::setGlyphColor(const Color &color)
	{
		_glyphColor = color;
	}

	void TextEdit::setOutlineColor(const Color &color)
	{
		_outlineColor = color;
	}

	void TextEdit::setCursorColor(const Color &color)
	{
		_cursorColor = color;
	}

	void TextEdit::setDepth(float depth)
	{
		_depth = depth;
	}

	TextEdit::ValidationState TextEdit::validationState() const
	{
		return _validate(_text);
	}

	bool TextEdit::hasText() const noexcept
	{
		return !_text.empty();
	}

	bool TextEdit::isObscured() const noexcept
	{
		return _obscured;
	}

	bool TextEdit::isEditEnabled() const noexcept
	{
		return _editEnabled;
	}

	bool TextEdit::isFocused() const noexcept
	{
		return _focused;
	}

	bool TextEdit::isHovered() const noexcept
	{
		return _hovered;
	}

	bool TextEdit::isCaretVisible() const noexcept
	{
		return _caretVisible;
	}

	Font::Text TextEdit::renderedText() const
	{
		return _text.empty() ? _placeholder : _editableRepresentation();
	}

	Font::Text TextEdit::visibleText() const
	{
		const unsigned int width = _innerSize().x;
		if (_text.empty())
		{
			return _fittingPrefix(_placeholder, width);
		}
		const Font::Text representation = _editableRepresentation();
		return representation.substr(_visibleStart, _visibleEnd - _visibleStart);
	}

	const Font::Text &TextEdit::text() const noexcept
	{
		return _text;
	}

	std::string TextEdit::textAsUTF8() const
	{
		return toUTF8(_text);
	}

	const Font::Text &TextEdit::placeholder() const noexcept
	{
		return _placeholder;
	}

	std::size_t TextEdit::cursor() const noexcept
	{
		return _cursor;
	}

	std::size_t TextEdit::visibleStart() const noexcept
	{
		return _visibleStart;
	}

	std::size_t TextEdit::visibleEnd() const noexcept
	{
		return _visibleEnd;
	}

	const SpriteSheet *TextEdit::spriteSheet() const noexcept
	{
		return _spriteSheet;
	}

	const Vector2Int &TextEdit::cornerSize() const noexcept
	{
		return _cornerSize;
	}

	Font *TextEdit::font() noexcept
	{
		return _font;
	}

	const Font *TextEdit::font() const noexcept
	{
		return _font;
	}

	const Font::Size &TextEdit::textSize() const noexcept
	{
		return _textSize;
	}

	const Color &TextEdit::glyphColor() const noexcept
	{
		return _glyphColor;
	}

	const Color &TextEdit::outlineColor() const noexcept
	{
		return _outlineColor;
	}

	const Color &TextEdit::cursorColor() const noexcept
	{
		return _cursorColor;
	}

	float TextEdit::depth() const noexcept
	{
		return _depth;
	}
}
