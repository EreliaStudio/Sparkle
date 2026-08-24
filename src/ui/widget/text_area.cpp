#include "ui/widget/text_area.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "rendering/command/text_render_command.hpp"

namespace spk
{
	TextArea::TextArea(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		activate();
	}

	TextArea::TextArea(std::string name, Font *font, Widget *parent) :
		TextArea(std::move(name), parent)
	{
		setFont(font);
	}

	std::vector<Font::Text> TextArea::_paragraphs() const
	{
		std::vector<Font::Text> result;
		Font::Text paragraph;
		for (Font::Codepoint codepoint : _text)
		{
			if (codepoint == U'\n')
			{
				result.push_back(std::move(paragraph));
				paragraph.clear();
			}
			else
			{
				paragraph.push_back(codepoint);
			}
		}
		result.push_back(std::move(paragraph));
		return result;
	}

	std::vector<Font::Text> TextArea::_wrapLines(unsigned int availableWidth) const
	{
		std::vector<Font::Text> result;
		if (_font == nullptr || _text.empty())
		{
			return result;
		}

		const unsigned int effectiveWidth = std::max(availableWidth, _minimalWidth);
		for (const Font::Text &paragraph : _paragraphs())
		{
			std::vector<Font::Text> words;
			for (std::size_t cursor = 0; cursor < paragraph.size();)
			{
				while (cursor < paragraph.size() && paragraph[cursor] == U' ')
				{
					++cursor;
				}
				const std::size_t start = cursor;
				while (cursor < paragraph.size() && paragraph[cursor] != U' ')
				{
					++cursor;
				}
				if (start != cursor)
				{
					words.emplace_back(paragraph.substr(start, cursor - start));
				}
			}

			if (words.empty())
			{
				result.emplace_back();
				continue;
			}

			Font::Text line;
			for (const Font::Text &word : words)
			{
				Font::Text candidate = line;
				if (!candidate.empty())
				{
					candidate.push_back(U' ');
				}
				candidate += word;

				if (line.empty() || _font->computeStringSize(candidate, _textSize).x <= effectiveWidth)
				{
					line = std::move(candidate);
				}
				else
				{
					result.push_back(std::move(line));
					line = word;
				}
			}
			result.push_back(std::move(line));
		}
		return result;
	}

	unsigned int TextArea::_lineHeight() const
	{
		return _font == nullptr ? 0 : _font->computeStringSize(U"Ajp|", _textSize).y;
	}

	unsigned int TextArea::_naturalWidth() const
	{
		if (_font == nullptr || _text.empty())
		{
			return _minimalWidth;
		}

		unsigned int result = _minimalWidth;
		for (const Font::Text &paragraph : _paragraphs())
		{
			result = std::max(result, _font->computeStringSize(paragraph, _textSize).x);
		}
		return result;
	}

	unsigned int TextArea::_blockHeight(std::size_t lineCount) const
	{
		if (lineCount == 0)
		{
			return 0;
		}
		return static_cast<unsigned int>(lineCount) * _lineHeight() + static_cast<unsigned int>(lineCount - 1) * _linePadding;
	}

	void TextArea::_updateSizeHint()
	{
		const Vector2UInt minimal = computePreferredSize(_minimalWidth);
		const Vector2UInt preferred = computePreferredSize(_naturalWidth());
		SizeHint hint = sizeHint();
		hint.minimal = Vector2(minimal);
		hint.preferred = Vector2(preferred);
		setSizeHint(hint);
	}

	void TextArea::_buildRenderSnapshot(RenderSnapshot::Builder &builder)
	{
		if (_font == nullptr || _text.empty() || geometry().width == 0 || geometry().height == 0)
		{
			return;
		}

		const std::vector<Font::Text> lines = _wrapLines(geometry().width);
		const int blockHeight = static_cast<int>(_blockHeight(lines.size()));
		int y = 0;
		if (_verticalAlignment == VerticalAlignment::Center)
		{
			y = (static_cast<int>(geometry().height) - blockHeight) / 2;
		}
		else if (_verticalAlignment == VerticalAlignment::Bottom)
		{
			y = static_cast<int>(geometry().height) - blockHeight;
		}

		int x = 0;
		if (_horizontalAlignment == HorizontalAlignment::Center)
		{
			x = static_cast<int>(geometry().width / 2);
		}
		else if (_horizontalAlignment == HorizontalAlignment::Right)
		{
			x = static_cast<int>(geometry().width);
		}

		auto &pass = builder.renderPass(Widget::OverlayKey);
		for (const Font::Text &line : lines)
		{
			if (!line.empty())
			{
				pass.emplace<TextRenderCommand>(
					_font,
					_textSize,
					line,
					TextRenderCommand::Anchor{
						.position = {x, y},
						.horizontalAlignment = _horizontalAlignment,
						.verticalAlignment = VerticalAlignment::Top},
					_glyphColor,
					_outlineColor,
					_depth);
			}
			y += static_cast<int>(_lineHeight() + _linePadding);
		}
	}

	void TextArea::setFont(Font *font)
	{
		if (font == nullptr)
		{
			throw std::invalid_argument("TextArea font cannot be null");
		}
		if (_font == font)
		{
			return;
		}
		_font = font;
		_updateSizeHint();
	}

	void TextArea::setText(Font::Text text)
	{
		if (_text == text)
		{
			return;
		}
		_text = std::move(text);
		_updateSizeHint();
	}

	void TextArea::setText(std::string_view text)
	{
		setText(Font::textFromUTF8(text));
	}

	void TextArea::setTextSize(const Font::Size &textSize)
	{
		if (_textSize == textSize)
		{
			return;
		}
		_textSize = textSize;
		_updateSizeHint();
	}

	void TextArea::setGlyphColor(const Color &color)
	{
		_glyphColor = color;
	}

	void TextArea::setOutlineColor(const Color &color)
	{
		_outlineColor = color;
	}

	void TextArea::setDepth(float depth)
	{
		_depth = depth;
	}

	void TextArea::setMinimalWidth(unsigned int width)
	{
		if (_minimalWidth == width)
		{
			return;
		}
		_minimalWidth = width;
		_updateSizeHint();
	}

	void TextArea::setLinePadding(unsigned int padding)
	{
		if (_linePadding == padding)
		{
			return;
		}
		_linePadding = padding;
		_updateSizeHint();
	}

	void TextArea::setHorizontalAlignment(HorizontalAlignment alignment)
	{
		_horizontalAlignment = alignment;
	}

	void TextArea::setVerticalAlignment(VerticalAlignment alignment)
	{
		_verticalAlignment = alignment;
	}

	void TextArea::setAlignment(HorizontalAlignment horizontal, VerticalAlignment vertical)
	{
		_horizontalAlignment = horizontal;
		_verticalAlignment = vertical;
	}

	Vector2UInt TextArea::computePreferredSize(unsigned int availableWidth) const
	{
		if (_font == nullptr)
		{
			return {0, 0};
		}
		if (_text.empty())
		{
			return {_minimalWidth, 0};
		}

		const std::vector<Font::Text> lines = _wrapLines(availableWidth);
		unsigned int width = _minimalWidth;
		for (const Font::Text &line : lines)
		{
			width = std::max(width, _font->computeStringSize(line, _textSize).x);
		}
		return {width, _blockHeight(lines.size())};
	}

	Font *TextArea::font() noexcept
	{
		return _font;
	}

	const Font *TextArea::font() const noexcept
	{
		return _font;
	}

	const Font::Text &TextArea::text() const noexcept
	{
		return _text;
	}

	const Font::Size &TextArea::textSize() const noexcept
	{
		return _textSize;
	}

	const Color &TextArea::glyphColor() const noexcept
	{
		return _glyphColor;
	}

	const Color &TextArea::outlineColor() const noexcept
	{
		return _outlineColor;
	}

	float TextArea::depth() const noexcept
	{
		return _depth;
	}

	unsigned int TextArea::minimalWidth() const noexcept
	{
		return _minimalWidth;
	}

	unsigned int TextArea::linePadding() const noexcept
	{
		return _linePadding;
	}

	HorizontalAlignment TextArea::horizontalAlignment() const noexcept
	{
		return _horizontalAlignment;
	}

	VerticalAlignment TextArea::verticalAlignment() const noexcept
	{
		return _verticalAlignment;
	}
}
