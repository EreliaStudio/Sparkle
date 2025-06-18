#include "widget/spk_text_area.hpp"

#include <cwctype>

#include "spk_generated_resources.hpp"

namespace spk
{
	spk::SpriteSheet defaultHoverNineSlice = spk::SpriteSheet::fromRawData(
		SPARKLE_GET_RESOURCE("resources/textures/defaultNineSlice_Light.png"), spk::Vector2UInt(3, 3), SpriteSheet::Filtering::Linear);

	void TextArea::_onGeometryChange()
	{
		spk::Geometry2D backgroundGeometry = {0, geometry().size};
		spk::Geometry2D layoutGeometry = backgroundGeometry.shrink(_backgroundFrame.cornerSize());
		_backgroundFrame.setGeometry(backgroundGeometry);

		_populateLabels(layoutGeometry.size);
		_composeLayout();

		_layout.setGeometry(layoutGeometry);
	}

	static std::vector<std::wstring> composeWordCollection(const std::wstring& p_text)
	{
		std::vector<std::wstring> result;
		const std::size_t n = p_text.size();

		std::size_t i = 0;
		while (i < n)
		{
			wchar_t c = p_text[i];

			if (c == L'\r' || c == L'\n')
			{
				if (c == L'\r' && i + 1 < n && p_text[i + 1] == L'\n')
				{
					++i;
				}

				result.emplace_back(1, L'\n');
				++i;
				continue;
			}

			std::wstring token;

			while (i < n && p_text[i] != L'\n' &&
				std::iswspace(static_cast<wint_t>(p_text[i])) == 0)
			{
				token += p_text[i];
				i++;
			}

			while (i < n && p_text[i] != L'\n' &&
				std::iswspace(static_cast<wint_t>(p_text[i])) != 0)
			{
				token += p_text[i];
				i++;
			}

			if (!token.empty())
			{
				result.emplace_back(std::move(token));
			}
		}

		return result;
	}

	void TextArea::_createNewLine()
	{
		std::unique_ptr<spk::TextLabel> newLabel = std::make_unique<spk::TextLabel>(name() + L"/LabelLine[" + std::to_wstring(_nbLine) + L"]", this);

		newLabel->setLayer(_backgroundFrame.layer() + 1);
		newLabel->setNineSlice(nullptr);
		newLabel->setCornerSize(0);
		newLabel->setFont(_font);
		newLabel->setFontSize(_fontSize);
		newLabel->setFontColor(_glyphColor, _outlineColor);

		newLabel->setTextAlignment(_horizontalAlignment, spk::VerticalAlignment::Centered);

		_labels.push_back(std::move(newLabel));
	}

	void TextArea::_insertLine(const std::wstring& p_text)
	{
		if (_nbLine >= _labels.size())
		{
			_createNewLine();
		}

		_labels[_nbLine]->setText(p_text);

		_nbLine++;
	}

    std::wstring trimSpaces(const std::wstring& p_text)
    {
		std::wstring result = p_text;

        while (std::iswspace(result.back()))
		{
			result.pop_back();
		}

		return (result);
    }

	void TextArea::_populateLabels(const spk::Vector2UInt &p_availableSize)
	{
		_nbLine = 0;
		std::wstring currentLine;

		const std::vector<std::wstring> tokens = composeWordCollection(_text);

		for (const std::wstring& token : tokens)
		{
			if (token == L"\n")
			{
				_insertLine(currentLine.empty() == true ? L" " : currentLine);
				currentLine.clear();
				continue;
			}

			std::wstring candidate = currentLine + token;
			spk::Vector2UInt tmpSize = _font->computeStringSize(candidate, _fontSize);

			if (tmpSize.x < p_availableSize.x)
			{
				currentLine = std::move(candidate);
				continue;
			}

			std::wstring trimmedCandidate = trimSpaces(candidate);
			spk::Vector2UInt trimmedSize = _font->computeStringSize(trimmedCandidate, _fontSize);

			if (trimmedSize.x < p_availableSize.x)
			{
				_insertLine(trimmedCandidate);
				currentLine = L"";
				continue;
			}

			_insertLine(currentLine);
			currentLine = token;
		}

		trimSpaces(currentLine);
		if (currentLine.empty() == false)
		{
			_insertLine(currentLine);
		}
	}

	void TextArea::_composeLayout()
	{
		_layout.clear();

		if (_verticalAlignment != spk::VerticalAlignment::Top)
		{
			_topSpacer.activate();
			_layout.addWidget(&_topSpacer, spk::Layout::SizePolicy::Extend);
		}
		else
		{
			_topSpacer.deactivate();
		}

		for (size_t i = 0; i < _labels.size(); i++)
		{
			if (i >= _nbLine)
			{
				_labels[i]->deactivate();
			}
			else
			{
				_labels[i]->activate();
				_layout.addWidget(_labels[i].get(), spk::Layout::SizePolicy::Minimum);
			}
		}

		if (_verticalAlignment != spk::VerticalAlignment::Down)
		{
			_downSpacer.activate();
			_layout.addWidget(&_downSpacer, spk::Layout::SizePolicy::Extend);
		}
		else
		{
			_downSpacer.deactivate();
		}
	}

	TextArea::TextArea(const std::wstring& p_name, spk::SafePointer<Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_backgroundFrame(p_name + L"/BackgroundFrame", this),
		_topSpacer(p_name + L"/TopSpacer", this),
		_downSpacer(p_name + L"/DownSpacer", this)
	{
		_layout.setElementPadding({0, 0});

		_backgroundFrame.setNineSlice(&defaultHoverNineSlice);
		_backgroundFrame.setCornerSize(_cornerSize);
		_backgroundFrame.activate();

		_font = Widget::defaultFont();
	}

	void TextArea::setLinePadding(size_t p_nbPixels)
	{
		_layout.setElementPadding({0, p_nbPixels});
	}

	size_t TextArea::linePadding() const
	{
		return _layout.elementPadding().y;
	}

	void TextArea::setText(const std::wstring& p_text)
	{
		_text = p_text;
		requireGeometryUpdate();
	}

	const std::wstring& TextArea::text() const
	{
		return _text;
	}

	void TextArea::setTextAlignment(const spk::HorizontalAlignment& p_horizontalAlignment, const spk::VerticalAlignment& p_verticalAlignment)
	{
		_horizontalAlignment = p_horizontalAlignment;
		_verticalAlignment = p_verticalAlignment;

		for (auto& label : _labels)
		{
			label->setTextAlignment(p_horizontalAlignment, spk::VerticalAlignment::Centered);
		}

		requireGeometryUpdate();
	}

	spk::HorizontalAlignment TextArea::horizontalAlignment() const
	{
		return _horizontalAlignment;
	}

	spk::VerticalAlignment TextArea::verticalAlignment() const
	{
		return _verticalAlignment;
	}

	void TextArea::setFont(spk::SafePointer<spk::Font> p_font)
	{
		_font = p_font;

		for (auto& lbl : _labels)
		{
			lbl->setFont(_font);
		}

		requireGeometryUpdate();
	}

	spk::SafePointer<spk::Font> TextArea::font() const
	{
		return _font;
	}

	void TextArea::setFontSize(const spk::Font::Size& p_size)
	{
		_fontSize = p_size;

		for (auto& lbl : _labels)
		{
			lbl->setFontSize(_fontSize);
		}

		requireGeometryUpdate();
	}

	spk::Font::Size TextArea::fontSize() const
	{
		return _fontSize;
	}

	void TextArea::setFontColor(const spk::Color& p_glyphColor, const spk::Color& p_outlineColor)
	{
		_glyphColor = p_glyphColor;
		_outlineColor = p_outlineColor;

		for (auto& lbl : _labels)
		{
			lbl->setFontColor(_glyphColor, _outlineColor);
		}
	}

	spk::Color TextArea::glyphColor() const
	{
		return _glyphColor;
	}

	spk::Color TextArea::outlineColor() const
	{
		return _outlineColor;
	}

	void TextArea::setCornerSize(const spk::Vector2UInt& p_cornerSize)
	{
		_cornerSize = p_cornerSize;
		_backgroundFrame.setCornerSize(_cornerSize);
		requireGeometryUpdate();
	}

	spk::Vector2UInt TextArea::cornerSize() const
	{
		return _cornerSize;
	}

	void TextArea::setNineSlice(spk::SafePointer<const spk::SpriteSheet> p_spriteSheet)
	{
		_backgroundFrame.setNineSlice(p_spriteSheet);
	}

	spk::SafePointer<const spk::SpriteSheet> TextArea::nineSlice() const
	{
		return _backgroundFrame.nineslice();
	}

	spk::Vector2UInt TextArea::computeMinimalSize(const size_t& p_minimalWidth)
	{
		spk::Vector2UInt result{0, 0};

		const uint32_t horizontalMargin = _backgroundFrame.cornerSize().x * 2;
		const uint32_t verticalMargin   = _backgroundFrame.cornerSize().y * 2;
		const uint32_t padding          = static_cast<uint32_t>(_layout.elementPadding().y);

		const size_t availableWidth = (p_minimalWidth > horizontalMargin)
										? (p_minimalWidth - horizontalMargin)
										: 0;

		if (_font == nullptr)
		{
			return result;
		}

		const std::vector<std::wstring> tokens = composeWordCollection(_text);

		std::wstring currentLine;
		size_t       maxLineWidth = 0U;

		const auto flushLine = [&](spk::Vector2UInt p_textSize)
		{
			if (p_textSize.x != 0)
			{
				maxLineWidth = std::max(maxLineWidth, static_cast<size_t>(p_textSize.x));
			}

			if (result.y != 0)
			{
				result.y += padding;
			}

			result.y += p_textSize.y;
			currentLine.clear();
		};

		for (const std::wstring& token : tokens)
		{
			spk::Vector2UInt currentLineSize = _font->computeStringSize(currentLine, _fontSize);

			if (token == L"\n")
			{
				flushLine(currentLineSize);
				continue;
			}

			std::wstring candidate = currentLine + token;
			spk::Vector2UInt candidateSize = _font->computeStringSize(candidate, _fontSize);

			if (candidateSize.x < availableWidth)
			{
				currentLine = std::move(candidate);
				continue;
			}

			std::wstring trimmedCandidate = trimSpaces(candidate);
			spk::Vector2UInt trimmedSize   = _font->computeStringSize(trimmedCandidate, _fontSize);

			if (trimmedSize.x < availableWidth)
			{
				flushLine(trimmedSize);
				continue;
			}

			flushLine(currentLineSize);
			currentLine = token;
		}

		if (!trimSpaces(currentLine).empty())
		{
			spk::Vector2UInt currentLineSize = _font->computeStringSize(currentLine, _fontSize);
			flushLine(currentLineSize);
		}

		result.x = static_cast<uint32_t>(maxLineWidth + horizontalMargin);

		result.y += verticalMargin;

		return result;
	}

	void TextArea::setMinimalWidth(const size_t& p_minimalWidth)
	{
		setMinimalSize(computeMinimalSize(p_minimalWidth));
	}
} 
