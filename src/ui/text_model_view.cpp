#include "ui/text_model_view.hpp"

#include <stdexcept>
#include <utility>

#include "ui/widget.hpp"

namespace spk
{
	TextModelDelegate::TextModelDelegate(Font *font) :
		_font(font != nullptr ? font : Widget::defaultStyle->font.get())
	{
	}

	void TextModelDelegate::setFont(Font *font)
	{
		if (font == nullptr)
		{
			throw std::invalid_argument("TextModelDelegate font cannot be null");
		}
		_font = font;
	}

	void TextModelDelegate::setTextSize(const Font::Size &size)
	{
		_textSize = size;
	}

	void TextModelDelegate::setGlyphColor(const Color &color)
	{
		_glyphColor = color;
	}

	void TextModelDelegate::setOutlineColor(const Color &color)
	{
		_outlineColor = color;
	}

	void TextModelDelegate::setPadding(const Vector2UInt &padding)
	{
		_padding = padding;
	}

	void TextModelDelegate::setRowHeight(unsigned int height)
	{
		_rowHeight = height;
	}

	std::unique_ptr<Widget> TextModelDelegate::createItem(std::string name, Widget *parent)
	{
		return std::make_unique<TextLabel>(std::move(name), parent);
	}

	void TextModelDelegate::bindItem(Widget &item, const DataModel<std::string> &model, std::size_t row, bool)
	{
		auto *label = dynamic_cast<TextLabel *>(&item);
		if (label == nullptr)
		{
			throw std::invalid_argument("TextModelDelegate requires a TextLabel item");
		}
		if (_font != nullptr)
		{
			label->setFont(_font);
		}
		label->setText(model.data(row));
		label->setTextSize(_textSize);
		label->setGlyphColor(_glyphColor);
		label->setOutlineColor(_outlineColor);
		label->setPadding(_padding);
		label->setAlignment({Alignment::Horizontal::Left, Alignment::Vertical::Center});
	}

	unsigned int TextModelDelegate::rowExtent(const DataModel<std::string> &, std::size_t) const
	{
		return _rowHeight;
	}

	TextModelView::TextModelView(std::string name, DataModel<std::string> *model, Widget *parent) :
		ModelView<std::string>(std::move(name), model, parent)
	{
		setDelegate(&_defaultDelegate);
	}

	TextModelDelegate &TextModelView::defaultDelegate() noexcept
	{
		return _defaultDelegate;
	}

	const TextModelDelegate &TextModelView::defaultDelegate() const noexcept
	{
		return _defaultDelegate;
	}
}
