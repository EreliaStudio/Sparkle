#include "ui/widget/prompt_panel.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace
{
	constexpr float Unlimited = static_cast<float>(std::numeric_limits<std::size_t>::max());
	constexpr unsigned int MessageMeasureWidth = 200;
	constexpr spk::Vector2UInt ElementPadding{8, 8};

	unsigned int _dimension(float value)
	{
		return value <= 0.0f ? 0u : static_cast<unsigned int>(std::ceil(value));
	}
}

namespace spk
{
	PromptPanel::PromptPanel(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_background(this->name() + ".background", this),
		_textArea(this->name() + ".message", &_background),
		_commandPanel(this->name() + ".commands", &_background)
	{
		applyStyle(defaultStyle);
		setMaximalSize({Unlimited, Unlimited});
		_textArea.setMaximalSize({Unlimited, Unlimited});
		_commandPanel.setMaximalSize({Unlimited, Unlimited});
		_layout.setElementPadding(ElementPadding);
		_textSizeContract = _textArea.subscribeToSizeHintEdition([this](ResizeableTrait *) {
			_updateSizeHint();
		});
		_commandSizeContract = _commandPanel.subscribeToSizeHintEdition([this](ResizeableTrait *) {
			_updateSizeHint();
		});
		_layoutReady = true;
		_rebuildLayout();
		activate();
	}

	void PromptPanel::applyStyle(const Style &style)
	{
		if (style.darkNineSlice != nullptr)
		{
			_background.setSpriteSheet(style.darkNineSlice.get());
		}
		_background.setCornerSize({16, 16});
		_textArea.applyStyle(style);
	}

	void PromptPanel::_rebuildLayout()
	{
		if (!_layoutReady)
		{
			return;
		}

		_layout.clear();
		if (!_textArea.text().empty())
		{
			_layout.addWidget(&_textArea, {Layout::SizePolicy::Extend, Layout::SizePolicy::Extend});
		}
		Layout::Element *commands = _layout.addWidget(
			&_commandPanel,
			{Layout::SizePolicy::Extend, Layout::SizePolicy::Minimum});
		commands->setHorizontalAlignment(Alignment::Horizontal::Right);
		_updateSizeHint();
		_onGeometryChange();
	}

	void PromptPanel::_updateSizeHint()
	{
		if (!_layoutReady)
		{
			return;
		}

		const Vector2 commandMinimum = _commandPanel.minimalSize();
		Vector2 content = commandMinimum;
		if (!_textArea.text().empty())
		{
			const unsigned int measureWidth = std::max(_dimension(commandMinimum.x), MessageMeasureWidth);
			const Vector2UInt textSize = _textArea.computePreferredSize(measureWidth);
			content.x = std::max(commandMinimum.x, static_cast<float>(textSize.x));
			content.y = static_cast<float>(textSize.y + ElementPadding.y) + commandMinimum.y;
		}

		const Vector2Int corner = _background.cornerSize();
		content.x += static_cast<float>(2 * std::max(corner.x, 0));
		content.y += static_cast<float>(2 * std::max(corner.y, 0));
		SizeHint hint = sizeHint();
		hint.minimal = content;
		hint.preferred = content;
		setSizeHint(hint);
		_onGeometryChange();
	}

	void PromptPanel::_onGeometryChange()
	{
		if (!_layoutReady)
		{
			return;
		}

		_background.setGeometry(Rect2D{Vector2Int{0, 0}, geometry().size});
		const Vector2Int corner = _background.cornerSize();
		const unsigned int horizontal = std::min(
			static_cast<unsigned int>(std::max(corner.x, 0)),
			geometry().width / 2);
		const unsigned int vertical = std::min(
			static_cast<unsigned int>(std::max(corner.y, 0)),
			geometry().height / 2);
		_layout.setGeometry(Rect2D{Vector2Int{static_cast<int>(horizontal), static_cast<int>(vertical)}, Vector2UInt{geometry().width - 2 * horizontal, geometry().height - 2 * vertical}});
	}

	void PromptPanel::setMessage(Font::Text message)
	{
		const bool wasEmpty = _textArea.text().empty();
		_textArea.setText(std::move(message));
		if (wasEmpty != _textArea.text().empty())
		{
			_rebuildLayout();
		}
	}

	void PromptPanel::setMessage(std::string_view message)
	{
		setMessage(Font::textFromUTF8(message));
	}

	const Font::Text &PromptPanel::message() const noexcept
	{
		return _textArea.text();
	}

	PushButton &PromptPanel::addButton(std::string name, std::string_view label)
	{
		return _commandPanel.addButton(std::move(name), label);
	}

	PushButton &PromptPanel::button(std::string_view name)
	{
		return _commandPanel.button(name);
	}

	const PushButton &PromptPanel::button(std::string_view name) const
	{
		return _commandPanel.button(name);
	}

	void PromptPanel::removeButton(std::string_view name)
	{
		_commandPanel.removeButton(name);
	}

	std::size_t PromptPanel::nbButton() const noexcept
	{
		return _commandPanel.nbButton();
	}

	PushButton::ClickContract PromptPanel::subscribe(std::string_view name, PushButton::ClickCallback callback)
	{
		return _commandPanel.subscribe(name, std::move(callback));
	}

	void PromptPanel::setButtonSizePolicy(Layout::SizeSettings sizePolicy)
	{
		_commandPanel.setSizePolicy(sizePolicy);
	}

	const Layout::SizeSettings &PromptPanel::buttonSizePolicy() const noexcept
	{
		return _commandPanel.sizePolicy();
	}

	void PromptPanel::setButtonPadding(const Vector2UInt &padding)
	{
		_commandPanel.setElementPadding(padding);
	}

	const Vector2UInt &PromptPanel::buttonPadding() const noexcept
	{
		return _commandPanel.elementPadding();
	}

	Panel &PromptPanel::background() noexcept
	{
		return _background;
	}

	const Panel &PromptPanel::background() const noexcept
	{
		return _background;
	}

	TextArea &PromptPanel::textArea() noexcept
	{
		return _textArea;
	}

	const TextArea &PromptPanel::textArea() const noexcept
	{
		return _textArea;
	}

	CommandPanel &PromptPanel::commandPanel() noexcept
	{
		return _commandPanel;
	}

	const CommandPanel &PromptPanel::commandPanel() const noexcept
	{
		return _commandPanel;
	}
}
