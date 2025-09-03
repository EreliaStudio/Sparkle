#include "widget/spk_debug_overlay.hpp"

namespace spk
{
	void DebugOverlay::_compose()
	{
		_layout.clear();
		_layout.setElementPadding({4, 4});

		for (size_t r = 0; r < _rows.size(); ++r)
		{
			Row &row = _rows[r];
			row.layout.clear();

			for (size_t c = 0; c < row.labels.size(); ++c)
			{
				spk::SafePointer<spk::TextLabel> lbl = row.labels[c].get();
				if (lbl != nullptr)
				{
					row.layout.addWidget(lbl, spk::Layout::SizePolicy::Extend);
				}
			}

			_layout.addLayout(&row.layout, spk::Layout::SizePolicy::Extend);
		}
	}

	void DebugOverlay::_applyFontsToFit()
	{
		for (size_t r = 0; r < _rows.size(); ++r)
		{
			Row &row = _rows[r];
			for (size_t c = 0; c < row.labels.size(); ++c)
			{
				spk::SafePointer<spk::TextLabel> lbl = row.labels[c].get();
				if (lbl == nullptr)
				{
					continue;
				}

				spk::Vector2UInt area = lbl->geometry().size;
				spk::SafePointer<spk::Font> f = lbl->font();
				if (f == nullptr)
				{
					continue;
				}

				spk::Font::Size opt = f->computeOptimalTextSize(lbl->text(), 0, area);

				size_t glyphNoOutline = 0;
				if (opt.glyph > _outlineSize * 2)
				{
					glyphNoOutline = opt.glyph - (_outlineSize * 2);
				}
				if (_maxGlyphSize > 0 && glyphNoOutline > _maxGlyphSize)
				{
					glyphNoOutline = _maxGlyphSize;
				}
				lbl->setFontSize(spk::Font::Size(glyphNoOutline, _outlineSize));
				lbl->setFontGlyphSharpness(_fontGlyphSharpness);
				lbl->setFontOutlineSharpness(_outlineGlyphSharpness);
			}
		}
	}

	void DebugOverlay::_ensureRow(const size_t &p_row)
	{
		if (p_row < _rows.size())
		{
			return;
		}

		const size_t oldSize = _rows.size();
		_rows.resize(p_row + 1);
	}

	void DebugOverlay::_ensureCol(const size_t &p_row, const size_t &p_col)
	{
		_ensureRow(p_row);
		Row &row = _rows[p_row];
		if (p_col < row.labels.size())
		{
			return;
		}

		const size_t oldSize = row.labels.size();
		row.labels.resize(p_col + 1);
		for (size_t c = oldSize; c <= p_col; ++c)
		{
			std::unique_ptr<spk::TextLabel> newLabel =
				std::make_unique<spk::TextLabel>(name() + L" - Label [" + std::to_wstring(c) + L"][" + std::to_wstring(p_row) + L"]", this);
			newLabel->setNineSlice(nullptr);
			newLabel->setFont(_font != nullptr ? _font : Widget::defaultFont());
			newLabel->setFontColor(_glyphColor, _outlineColor);
			newLabel->activate();
			row.labels[c] = std::move(newLabel);
		}
	}

	void DebugOverlay::_onGeometryChange()
	{
		_compose();
		_layout.setGeometry({0, geometry().size});
		_applyFontsToFit();
	}

	DebugOverlay::DebugOverlay(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		
	}

	void DebugOverlay::configureRows(const size_t &p_rows, const size_t &p_defaultColumns)
	{
		_rows.clear();
		_rows.resize(p_rows);
		for (size_t r = 0; r < p_rows; ++r)
		{
			setRowColumns(r, p_defaultColumns);
		}
		requireGeometryUpdate();
	}

	void DebugOverlay::setRowColumns(const size_t &p_row, const size_t &p_columns)
	{
		_ensureRow(p_row);
		Row &row = _rows[p_row];

		const size_t current = row.labels.size();
		if (p_columns == current)
		{
			return;
		}

		if (p_columns < current)
		{
			row.labels.resize(p_columns);
		}
		else
		{
			row.labels.resize(p_columns);
			for (size_t c = current; c < p_columns; ++c)
			{
				std::unique_ptr<spk::TextLabel> newLabel =
					std::make_unique<spk::TextLabel>(name() + L" - Label [" + std::to_wstring(c) + L"][" + std::to_wstring(p_row) + L"]", this);
				newLabel->setNineSlice(nullptr);
				newLabel->setFont(_font != nullptr ? _font : Widget::defaultFont());
				newLabel->setFontColor(_glyphColor, _outlineColor);
				newLabel->activate();
				row.labels[c] = std::move(newLabel);
			}
		}

		requireGeometryUpdate();
	}

	void DebugOverlay::setText(const size_t &p_row, const size_t &p_col, const std::wstring &p_text)
	{
		_ensureCol(p_row, p_col);
		_rows[p_row].labels[p_col]->setText(p_text);
		requireGeometryUpdate();
	}

	void DebugOverlay::setFont(spk::SafePointer<spk::Font> p_font)
	{
		_font = p_font;
		for (auto &row : _rows)
		{
			for (auto &lbl : row.labels)
			{
				if (lbl != nullptr)
				{
					lbl->setFont(_font != nullptr ? _font : Widget::defaultFont());
				}
			}
		}
		requireGeometryUpdate();
	}

	void DebugOverlay::setFontColor(const spk::Color &p_glyphColor, const spk::Color &p_outlineColor)
	{
		_glyphColor = p_glyphColor;
		_outlineColor = p_outlineColor;
		for (auto &row : _rows)
		{
			for (auto &lbl : row.labels)
			{
				if (lbl != nullptr)
				{
					lbl->setFontColor(_glyphColor, _outlineColor);
				}
			}
		}
	}

	void DebugOverlay::setFontOutlineSize(size_t p_outlineSize)
	{
		_outlineSize = p_outlineSize;
		requireGeometryUpdate();
	}

	void DebugOverlay::setFontGlyphSharpness(size_t p_pixels)
	{
		_fontGlyphSharpness = p_pixels;
	}

	void DebugOverlay::setFontOutlineSharpness(size_t p_pixels)
	{
		_outlineGlyphSharpness = p_pixels;
	}

	void DebugOverlay::setMaxGlyphSize(size_t p_maxGlyphSize)
	{
		_maxGlyphSize = p_maxGlyphSize;
		requireGeometryUpdate();
	}

	uint32_t DebugOverlay::computeMaxHeightPixels() const
	{
		if (_rows.empty())
		{
			return std::numeric_limits<uint32_t>::max();
		}

		const spk::SafePointer<spk::Font> f = (_font != nullptr ? _font : Widget::defaultFont());
		const size_t glyph = (_maxGlyphSize > 0 ? _maxGlyphSize : 0);

		if (glyph == 0 || f == nullptr)
		{
			return std::numeric_limits<uint32_t>::max();
		}

		uint32_t cornerY = 0u;
		for (const auto &row : _rows)
		{
			if (!row.labels.empty() && row.labels[0] != nullptr)
			{
				cornerY = row.labels[0]->cornerSize().y;
				break;
			}
		}

		spk::Vector2UInt textSize = f->computeStringSize(L"A", spk::Font::Size(glyph, _outlineSize));
		uint32_t perRow = textSize.y + cornerY * 2u;
		uint32_t total = static_cast<uint32_t>(_rows.size()) * perRow;
		if (_rows.size() > 1)
		{
			uint32_t pad = _layout.elementPadding().y;
			total += pad * static_cast<uint32_t>(_rows.size() - 1);
		}
		return total;
	}
}
