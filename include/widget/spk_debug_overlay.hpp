
#pragma once

#include "widget/spk_linear_layout.hpp"
#include "widget/spk_text_label.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	class DebugOverlay : public spk::Widget
	{
	private:
		struct Row
		{
			spk::HorizontalLayout layout;
			std::vector<std::unique_ptr<spk::TextLabel>> labels;
		};

		spk::VerticalLayout _layout;
		std::vector<Row> _rows;

		size_t _outlineSize = 0;
		size_t _maxGlyphSize = 0;
		size_t _fontGlyphSharpness = 1;
		size_t _outlineGlyphSharpness = 1;
		spk::SafePointer<spk::Font> _font = nullptr;
		spk::Color _glyphColor = spk::Color::white;
		spk::Color _outlineColor = spk::Color::black;

		void _compose();
		void _applyFontsToFit();
		void _ensureRow(const size_t &p_row);
		void _ensureCol(const size_t &p_row, const size_t &p_col);

		void _onGeometryChange() override;

	public:
		DebugOverlay(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);

		void configureRows(const size_t &p_rows, const size_t &p_defaultColumns);
		void setRowColumns(const size_t &p_row, const size_t &p_columns);

		void setText(const size_t &p_row, const size_t &p_col, const std::wstring &p_text);
		void setFont(spk::SafePointer<spk::Font> p_font);
		void setFontColor(const spk::Color &p_glyphColor, const spk::Color &p_outlineColor);
		void setFontOutlineSize(size_t p_outlineSize);
		void setFontGlyphSharpness(size_t p_pixels);
		void setFontOutlineSharpness(size_t p_pixels);

		void setMaxGlyphSize(size_t p_maxGlyphSize);
		uint32_t labelHeight() const;
		size_t maxHeightPixels() const;
		uint32_t computeMaxHeightPixels() const;
	};
}
