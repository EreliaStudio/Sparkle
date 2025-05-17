#pragma once

#include "widget/spk_text_label.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	template <size_t TColumn, size_t TRow>
	class DebugOverlay : public spk::Widget
	{
		static_assert(TColumn > 0, "DebugOverlay: TColumn must be greater than 0.");
		static_assert(TRow > 0, "DebugOverlay: TRow must be greater than 0.");

	public:
		static inline const size_t Column = TColumn;
		static inline const size_t Row = TRow;

	private:
		size_t _outlineSize = 0;
		std::unique_ptr<spk::TextLabel> _labels[Column][Row];

		void _onGeometryChange() override
		{
			spk::Font::Size fontSize = spk::Font::Size(geometry().size.y, _outlineSize);

			spk::Vector2UInt textLabelSize = geometry().size / spk::Vector2UInt(Column, Row);

			for (size_t i = 0; i < Column; i++)
			{
				for (size_t j = 0; j < Row; j++)
				{
					spk::Font::Size tmpSize = _labels[i][j]->font()->computeOptimalTextSize(_labels[i][j]->text(), 0, textLabelSize);

					fontSize.glyph = std::min(fontSize.glyph, tmpSize.glyph - _outlineSize);
				}
			}

			for (size_t i = 0; i < Column; i++)
			{
				for (size_t j = 0; j < Row; j++)
				{
					_labels[i][j]->setFontSize(fontSize);
					_labels[i][j]->setGeometry(textLabelSize * spk::Vector2Int(i, j), textLabelSize);
				}
			}
		}

	public:
		DebugOverlay(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent)
		{
			for (size_t i = 0; i < Column; i++)
			{
				for (size_t j = 0; j < Row; j++)
				{
					_labels[i][j] =
						std::make_unique<spk::TextLabel>(p_name + L" - Label [" + std::to_wstring(i) + L"][" + std::to_wstring(j) + L"]", this);
					_labels[i][j]->setNineSlice(nullptr);
					_labels[i][j]->activate();
				}
			}
		}

		void setText(const size_t &p_col, const size_t &p_row, const std::wstring &p_text)
		{
			if (p_col >= Column || p_row >= Row)
			{
				throw std::invalid_argument("Can't set debug overlay [" + std::to_string(Column) + " / " + std::to_string(Row) +
											"] text at coordinate [" + std::to_string(p_col) + " / " + std::to_string(p_row) + "]");
			}
			_labels[p_col][p_row]->setText(p_text);
		}

		void setFont(spk::SafePointer<spk::Font> p_font)
		{
			for (size_t i = 0; i < Column; i++)
			{
				for (size_t j = 0; j < Row; j++)
				{
					_labels[i][j]->setFont(p_font);
				}
			}
		}

		void setFontColor(const spk::Color &p_glyphColor, const spk::Color &p_outlineColor)
		{
			for (size_t i = 0; i < Column; i++)
			{
				for (size_t j = 0; j < Row; j++)
				{
					_labels[i][j]->setFontColor(p_glyphColor, p_outlineColor);
				}
			}
		}

		void setFontOutlineSize(size_t p_outlineSize)
		{
			_outlineSize = p_outlineSize;
			requireGeometryUpdate();
		}
	};
}