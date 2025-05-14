#include "widget/spk_multiline_text_label.hpp"

#include <algorithm>

namespace spk
{
	static std::vector<std::wstring> splitLines(const std::wstring &p_str)
	{
		std::vector<std::wstring> out;
		size_t start = 0, pos;
		while ((pos = p_str.find(L"\n", start)) != std::wstring::npos)
		{
			out.push_back(p_str.substr(start, pos - start));
			start = pos + 1;
		}
		out.push_back(p_str.substr(start));
		return out;
	}

	MultilineTextLabel::MultilineTextLabel(const std::wstring &p_name,
								 spk::SafePointer<spk::Widget> p_parent) :
			Widget(p_name, p_parent)
	{
		setNineSlice(Widget::defaultNineSlice());
		setFont(Widget::defaultFont());
		setTextColor(spk::Color::white, spk::Color::black);
	}

	void MultilineTextLabel::_refreshLineRenderers()
	{
		const auto lines = splitLines(_text);

		if (lines.size() != _lineRenderers.size())
		{
			_lineRenderers.clear();
			_contracts.clear();
			_lineRenderers.resize(lines.size());

			for (auto &r : _lineRenderers)
			{
				_contracts.push_back(r.subscribeToFontEdition([this]() { requireGeometryUpdate(); }));
			}
		}
	}

	void MultilineTextLabel::_onGeometryChange()
	{
		DEBUG_LINE();
		_backgroundRenderer.clear();
		_backgroundRenderer.prepare(geometry(), layer(), _cornerSize);
		_backgroundRenderer.validate();

		DEBUG_LINE();

		_refreshLineRenderers();

		DEBUG_LINE();
		const auto lines = splitLines(_text);
		DEBUG_LINE();
		if (lines.empty())
		{
		DEBUG_LINE();
			return;
		}
		DEBUG_LINE();

		const spk::Geometry2D innerRect = geometry().shrink(_cornerSize);

		DEBUG_LINE();
		std::vector<spk::Vector2UInt> lineSizes;
		lineSizes.reserve(lines.size());
		uint32_t totalHeight = 0;
		uint32_t maxWidth    = 0;

		DEBUG_LINE();
		for (const auto &ln : lines)
		{
		DEBUG_LINE();
			spk::Vector2UInt sz = _lineRenderers.front().font()->computeStringSize(ln, _lineRenderers.front().fontSize());
		DEBUG_LINE();
			lineSizes.push_back(sz);
			maxWidth    = std::max(maxWidth,   sz.x);
			totalHeight += sz.y;
		}
		DEBUG_LINE();

		int32_t startY = innerRect.anchor.y;
		DEBUG_LINE();
		if (_verticalAlignment == spk::VerticalAlignment::Centered)
		{
		DEBUG_LINE();
			startY += static_cast<int32_t>((innerRect.size.y - totalHeight) / 2);
		}
		else if (_verticalAlignment == spk::VerticalAlignment::Down)
		{
		DEBUG_LINE();
			startY += static_cast<int32_t>(innerRect.size.y - totalHeight);
		}

		DEBUG_LINE();
		for (size_t i = 0; i < lines.size(); ++i)
		{
		DEBUG_LINE();
			spk::FontRenderer &lineRenderer = _lineRenderers[i];
			lineRenderer.clear();
			lineRenderer.setFont(_lineRenderers.front().font());
			lineRenderer.setFontSize(_lineRenderers.front().fontSize());

		DEBUG_LINE();
			int32_t x = innerRect.anchor.x;
			if (_horizontalAlignment == spk::HorizontalAlignment::Centered)
			{
				x += static_cast<int32_t>((innerRect.size.x - lineSizes[i].x) / 2);
			}
			else if (_horizontalAlignment == spk::HorizontalAlignment::Right)
			{
				x += static_cast<int32_t>(innerRect.size.x - lineSizes[i].x);
			}
		DEBUG_LINE();

			int32_t y = startY;
			for (size_t j = 0; j < i; ++j)
			{
				y += lineSizes[j].y;
			}

		DEBUG_LINE();
			lineRenderer.prepare(lines[i], {x, y}, layer() + 0.01f);
			lineRenderer.validate();
		DEBUG_LINE();
		}
		DEBUG_LINE();
	}

	void MultilineTextLabel::_onPaintEvent(spk::PaintEvent & /*unused*/)
	{
		_backgroundRenderer.render();

		for (auto &lineRenderer : _lineRenderers)
		{
			lineRenderer.render();
		}
	}

	spk::Vector2UInt MultilineTextLabel::computeTextSize() const
	{
		if (_text == L"")
		{
			return (spk::Vector2UInt(0, 0));
		}
		std::vector<std::wstring> lines = splitLines(_text);
		if (lines.empty())
		{
			return (spk::Vector2UInt(0, 0));
		}

		uint32_t maxWidth = 0;
		uint32_t totalHeight = 0;

		for (const auto &ln : lines)
		{
			spk::Vector2UInt sz = font()->computeStringSize(ln, fontSize());
			maxWidth    = std::max(maxWidth,   sz.x);
			totalHeight += sz.y;
		}

		return {maxWidth, totalHeight};
	}

	spk::Vector2UInt MultilineTextLabel::computeExpectedTextSize(const spk::Font::Size &p_textSize) const
	{
		if (_text == L"")
		{
			return (spk::Vector2UInt(0, 0));
		}
		std::vector<std::wstring> lines = splitLines(_text);
		if (lines.empty())
		{
			return (spk::Vector2UInt(0, 0));
		}

		uint32_t maxWidth = 0, totalHeight = 0;
		for (const auto &ln : lines)
		{
			spk::Vector2UInt sz = font()->computeStringSize(ln, p_textSize);
			maxWidth    = std::max(maxWidth,   sz.x);
			totalHeight += sz.y;
		}
		return {maxWidth, totalHeight};
	}

	spk::Vector2UInt MultilineTextLabel::minimalSize() const
	{
		if (_text == L"")
		{
			return (spk::Vector2UInt(0, 0));
		}

		return (computeTextSize() + _cornerSize * 2);
	}

	spk::SafePointer<spk::Font> MultilineTextLabel::font() const
	{
		return _lineRenderers.empty() ? nullptr : _lineRenderers.front().font();
	}

	const spk::Font::Size& MultilineTextLabel::fontSize() const
	{
		static spk::Font::Size defaultFontSize = {16, 0};
		return _lineRenderers.empty() ? defaultFontSize : _lineRenderers.front().fontSize();
	}

	void MultilineTextLabel::setFont(spk::SafePointer<spk::Font> p_font)
	{
		for (auto &lineRenderer : _lineRenderers)
		{
			lineRenderer.setFont(p_font);
		}
		requireGeometryUpdate();
	}

	void MultilineTextLabel::setText(const std::wstring &p_text)
	{
		_text = p_text;
		requireGeometryUpdate();
	}

	void MultilineTextLabel::setFontSize(const spk::Font::Size &p_textSize)
	{
		for (auto &lineRenderer : _lineRenderers)
		{
			lineRenderer.setFontSize(p_textSize);
		}
		requireGeometryUpdate();
	}

	void MultilineTextLabel::setTextColor(const spk::Color &p_glyphColor, const spk::Color &p_outlineColor)
	{
		for (auto &lineRenderer : _lineRenderers)
		{
			lineRenderer.setGlyphColor(p_glyphColor);
			lineRenderer.setOutlineColor(p_outlineColor);
		}
	}

	void MultilineTextLabel::setTextAlignment(const spk::HorizontalAlignment &p_hAlign,
							  const spk::VerticalAlignment   &p_vAlign)
	{
		_horizontalAlignment = p_hAlign;
		_verticalAlignment   = p_vAlign;
		requireGeometryUpdate();
	}

	void MultilineTextLabel::setNineSlice(spk::SafePointer<const spk::SpriteSheet> p_spriteSheet)
	{
		_backgroundRenderer.setSpriteSheet(p_spriteSheet);
	}

	void MultilineTextLabel::setCornerSize(const spk::Vector2UInt &p_cornerSize)
	{
		_cornerSize = p_cornerSize;
		requireGeometryUpdate();
	}
}