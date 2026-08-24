#include "ui/widget/text_label.hpp"

#include <stdexcept>
#include <utility>

namespace spk
{
	TextLabel::TextLabel(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		activate();
	}

	TextLabel::TextLabel(std::string name, Font *font, Widget *parent) :
		TextLabel(std::move(name), parent)
	{
		setFont(font);
	}

	void TextLabel::_updateSizeHint()
	{
		Vector2 intrinsicSize{0.0f, 0.0f};
		if (_font != nullptr && !_text.empty())
		{
			const Vector2UInt measuredSize = _font->computeStringSize(_text, _textSize);
			intrinsicSize = Vector2{
				static_cast<float>(measuredSize.x + 2 * _padding.x),
				static_cast<float>(measuredSize.y + 2 * _padding.y)};
		}

		SizeHint hint = sizeHint();
		hint.minimal = intrinsicSize;
		hint.preferred = intrinsicSize;
		setSizeHint(hint);
	}

	TextRenderCommand::Anchor TextLabel::_textAnchor() const
	{
		Vector2Int position;
		switch (_horizontalAlignment)
		{
		case HorizontalAlignment::Left:
			position.x = static_cast<int>(_padding.x);
			break;
		case HorizontalAlignment::Center:
			position.x = static_cast<int>(geometry().width / 2);
			break;
		case HorizontalAlignment::Right:
			position.x = static_cast<int>(geometry().width) - static_cast<int>(_padding.x);
			break;
		}

		switch (_verticalAlignment)
		{
		case VerticalAlignment::Top:
			position.y = static_cast<int>(_padding.y);
			break;
		case VerticalAlignment::Center:
			position.y = static_cast<int>(geometry().height / 2);
			break;
		case VerticalAlignment::Bottom:
			position.y = static_cast<int>(geometry().height) - static_cast<int>(_padding.y);
			break;
		}

		return TextRenderCommand::Anchor{
			.position = position,
			.horizontalAlignment = _horizontalAlignment,
			.verticalAlignment = _verticalAlignment};
	}

	void TextLabel::_buildRenderSnapshot(RenderSnapshot::Builder &builder)
	{
		if (_font == nullptr || _text.empty() || geometry().width == 0 || geometry().height == 0)
		{
			return;
		}

		builder.renderPass(Widget::OverlayKey).emplace<TextRenderCommand>(_font, _textSize, _text, _textAnchor(), _glyphColor, _outlineColor, _depth);
	}

	void TextLabel::setFont(Font *font)
	{
		if (font == nullptr)
		{
			throw std::invalid_argument("TextLabel font cannot be null");
		}
		if (_font == font)
		{
			return;
		}
		_font = font;
		_updateSizeHint();
	}

	void TextLabel::setText(Font::Text text)
	{
		if (_text == text)
		{
			return;
		}
		_text = std::move(text);
		_updateSizeHint();
	}

	void TextLabel::setText(std::string_view text)
	{
		setText(Font::textFromUTF8(text));
	}

	void TextLabel::setTextSize(const Font::Size &textSize)
	{
		if (_textSize == textSize)
		{
			return;
		}
		_textSize = textSize;
		_updateSizeHint();
	}

	void TextLabel::setGlyphColor(const Color &color)
	{
		_glyphColor = color;
	}

	void TextLabel::setOutlineColor(const Color &color)
	{
		_outlineColor = color;
	}

	void TextLabel::setDepth(float depth)
	{
		_depth = depth;
	}

	void TextLabel::setHorizontalAlignment(HorizontalAlignment alignment)
	{
		_horizontalAlignment = alignment;
	}

	void TextLabel::setVerticalAlignment(VerticalAlignment alignment)
	{
		_verticalAlignment = alignment;
	}

	void TextLabel::setAlignment(HorizontalAlignment horizontal, VerticalAlignment vertical)
	{
		_horizontalAlignment = horizontal;
		_verticalAlignment = vertical;
	}

	void TextLabel::setPadding(const Vector2UInt &padding)
	{
		if (_padding == padding)
		{
			return;
		}
		_padding = padding;
		_updateSizeHint();
	}

	Font *TextLabel::font() noexcept
	{
		return _font;
	}

	const Font *TextLabel::font() const noexcept
	{
		return _font;
	}

	const Font::Text &TextLabel::text() const noexcept
	{
		return _text;
	}

	const Font::Size &TextLabel::textSize() const noexcept
	{
		return _textSize;
	}

	const Color &TextLabel::glyphColor() const noexcept
	{
		return _glyphColor;
	}

	const Color &TextLabel::outlineColor() const noexcept
	{
		return _outlineColor;
	}

	float TextLabel::depth() const noexcept
	{
		return _depth;
	}

	HorizontalAlignment TextLabel::horizontalAlignment() const noexcept
	{
		return _horizontalAlignment;
	}

	VerticalAlignment TextLabel::verticalAlignment() const noexcept
	{
		return _verticalAlignment;
	}

	const Vector2UInt &TextLabel::padding() const noexcept
	{
		return _padding;
	}
}
