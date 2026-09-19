#pragma once

#include <string>

#include "container/data_model.hpp"
#include "graphics/font.hpp"
#include "ui/model_view.hpp"
#include "ui/widget/text_label.hpp"

namespace spk
{
	class TextModelDelegate : public ModelView<std::string>::Delegate
	{
		Font *_font = nullptr;
		Font::Size _textSize{16};
		Color _glyphColor{1, 1, 1, 1};
		Color _outlineColor{0, 0, 0, 1};
		Vector2UInt _padding{4, 2};
		unsigned int _rowHeight = 20;

	public:
		explicit TextModelDelegate(Font *font = nullptr);
		void setFont(Font *font);
		void setTextSize(const Font::Size &size);
		void setGlyphColor(const Color &color);
		void setOutlineColor(const Color &color);
		void setPadding(const Vector2UInt &padding);
		void setRowHeight(unsigned int height);
		[[nodiscard]] std::unique_ptr<Widget> createItem(std::string name, Widget *parent) override;
		void bindItem(Widget &item, const DataModel<std::string> &model, std::size_t row, bool selected) override;
		[[nodiscard]] unsigned int rowExtent(const DataModel<std::string> &model, std::size_t row) const override;
	};

	class TextModelView : public ModelView<std::string>
	{
		TextModelDelegate _defaultDelegate;

	public:
		explicit TextModelView(std::string name, DataModel<std::string> *model, Widget *parent = nullptr);
		[[nodiscard]] TextModelDelegate &defaultDelegate() noexcept;
		[[nodiscard]] const TextModelDelegate &defaultDelegate() const noexcept;
	};
}
