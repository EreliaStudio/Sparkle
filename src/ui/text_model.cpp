#include "ui/text_model.hpp"

#include <stdexcept>
#include <utility>

#include "ui/widget.hpp"

namespace spk
{
	TextModel::Delegate::Delegate(Font *font) :
		_font(font != nullptr ? font : Widget::defaultStyle->font.get())
	{
	}

	void TextModel::Delegate::setFont(Font *font)
	{
		if (font == nullptr)
		{
			throw std::invalid_argument("TextModel delegate font cannot be null");
		}
		_font = font;
	}

	void TextModel::Delegate::setTextSize(const Font::Size &size)
	{
		_textSize = size;
	}

	void TextModel::Delegate::setGlyphColor(const Color &color)
	{
		_glyphColor = color;
	}

	void TextModel::Delegate::setOutlineColor(const Color &color)
	{
		_outlineColor = color;
	}

	void TextModel::Delegate::setPadding(const Vector2UInt &padding)
	{
		_padding = padding;
	}

	void TextModel::Delegate::setRowHeight(unsigned int height)
	{
		_rowHeight = height;
	}

	std::unique_ptr<Widget> TextModel::Delegate::createItem(std::string name, Widget *parent)
	{
		return std::make_unique<TextLabel>(std::move(name), parent);
	}

	void TextModel::Delegate::bindItem(Widget &item, const DataModel<std::string> &model, std::size_t row, bool)
	{
		auto *label = dynamic_cast<TextLabel *>(&item);
		if (label == nullptr)
		{
			throw std::invalid_argument("TextModel delegate requires a TextLabel item");
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

	unsigned int TextModel::Delegate::rowExtent(const DataModel<std::string> &, std::size_t) const
	{
		return _rowHeight;
	}

	TextModel::View::View(std::string name, Widget *parent) :
		DataModel<std::string>::View(std::move(name), parent)
	{
		setDelegate(&_defaultDelegate);
	}

	void TextModel::View::setModel(TextModel *model)
	{
		DataModel<std::string>::View::setModel(model);
	}

	TextModel::Delegate &TextModel::View::defaultDelegate() noexcept
	{
		return _defaultDelegate;
	}

	const TextModel::Delegate &TextModel::View::defaultDelegate() const noexcept
	{
		return _defaultDelegate;
	}
}
