#include "widget/spk_multiline_text_label.hpp"

#include <cwctype>

namespace spk
{
	void MultilineTextLabel::_onGeometryChange()
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

        /*----------- 1) Handle hard line breaks ------------------------*/
        if (c == L'\r' || c == L'\n')
        {
            // Treat CRLF as a single newline
            if (c == L'\r' && i + 1 < n && p_text[i + 1] == L'\n')
			{
                ++i;
			}

            result.emplace_back(1, L'\n');
            ++i;
            continue;
        }

        /*----------- 2) Build one “word + trailing-spaces” token -------*/
        std::wstring token;

        // 2a) word part  : read until white-space or newline
        while (i < n && p_text[i] != L'\n' &&
               !std::iswspace(static_cast<wint_t>(p_text[i])))
        {
            token += p_text[i++];
        }

        // 2b) trailing WS : read spaces/tabs/etc. but stop before '\n'
        while (i < n && p_text[i] != L'\n' &&
               std::iswspace(static_cast<wint_t>(p_text[i])))
        {
            token += p_text[i++];
        }

        if (!token.empty())
		{
			result.emplace_back(std::move(token));
		}
    }

    return result;
	}

	void MultilineTextLabel::_createNewLine()
	{
		std::unique_ptr<spk::TextLabel> newLabel = std::make_unique<spk::TextLabel>(name() + L"/LabelLine[" + std::to_wstring(_nbLine) + L"]", this);

		newLabel->setFont(_font);
		newLabel->setFontSize(_fontSize);

		_labels.push_back(std::move(newLabel));
	}

	void MultilineTextLabel::_insertLine(const std::wstring& p_text)
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

	void MultilineTextLabel::_populateLabels(const spk::Vector2UInt &p_availableSize)
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

	void MultilineTextLabel::_composeLayout()
	{
		_layout.clear();

		for (size_t i = 0; i < _labels.size(); i++)
		{
			if (i >= _nbLine)
			{
				continue;
			}

			_layout.addWidget(_labels[i].get(), spk::Layout::SizePolicy::Minimum);
		}
		_layout.addWidget(&_spacer, spk::Layout::SizePolicy::Extend);
		
		for (size_t i = 0; i < _labels.size(); i++)
		{
			if (i >= _nbLine)
			{
				_labels[i]->deactivate();
			}
			else
			{
				_labels[i]->activate();
			}
		}
		_spacer.activate();
	}

	MultilineTextLabel::MultilineTextLabel(const std::wstring& p_name, spk::SafePointer<Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_backgroundFrame(p_name + L"/BackgroundFrame", this),
		_spacer(p_name + L"/Spacer", this)
	{
		_backgroundFrame.activate();
		_font = Widget::defaultFont();
		_fontSize = {35, 0};
	}

	void MultilineTextLabel::setText(const std::wstring& p_text)
	{
		_text = p_text;
		requireGeometryUpdate();
	}

	Vector2UInt MultilineTextLabel::minimalSize() const
	{
		Vector2UInt minimalLabelSize = {0, 0};

		return minimalLabelSize + _backgroundFrame.cornerSize() * 2;
	}
} 
