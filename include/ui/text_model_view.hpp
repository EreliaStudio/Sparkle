#pragma once

#include <string>

#include "graphics/font.hpp"
#include "container/text_model.hpp"
#include "ui/data_model_view.hpp"
#include "ui/widget/text_label.hpp"

namespace spk
{
	class TextModel::Delegate : public DataModel<std::string>::View::Delegate
	{
		Font *_font = nullptr;
		Font::Size _textSize{16};
		Color _glyphColor{1, 1, 1, 1};
		Color _outlineColor{0, 0, 0, 1};
		Vector2UInt _padding{4, 2};
		unsigned int _rowHeight = 20;

	public:
		explicit Delegate(Font *font = nullptr);
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

	class TextModel::View : public DataModel<std::string>::View
	{
		TextModel::Delegate _defaultDelegate;

	public:
		explicit View(std::string name, Widget *parent = nullptr);
		void setModel(TextModel *model);
		[[nodiscard]] TextModel::Delegate &defaultDelegate() noexcept;
		[[nodiscard]] const TextModel::Delegate &defaultDelegate() const noexcept;
	};
}
