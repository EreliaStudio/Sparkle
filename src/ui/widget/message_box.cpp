#include "ui/widget/message_box.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	unsigned int dimension(float value) noexcept
	{
		return value <= 0.0f ? 0u : static_cast<unsigned int>(std::ceil(value));
	}
}

namespace spk
{
	MessageBox::MessageBox(std::string name, Widget *parent) :
		IInterfaceWindow(std::move(name), parent),
		_messageContent(this->name() + ".content", &normalBackground())
	{
		setContent(&_messageContent);
		_closeContract = subscribeToClose([this]() { close(); });
		_resizeContract = subscribeToResize([this](const Vector2UInt &) { _updateMinimumContentSize(); });
		_contentSizeContract = _messageContent.subscribeToSizeHintEdition([this](ResizeableTrait *) {
			_updateMinimumContentSize();
		});
		_updateMinimumContentSize();
	}

	void MessageBox::_updateMinimumContentSize()
	{
		const Vector2 commandMinimum = _messageContent.commandPanel().minimalSize();
		const unsigned int minimumWidth = std::max(_minimalWidth, dimension(commandMinimum.x));
		const unsigned int measureWidth = std::max(minimumWidth, _messageContent.geometry().width);
		Vector2 minimum{static_cast<float>(minimumWidth), commandMinimum.y};
		if (!_messageContent.textArea().text().empty())
		{
			const Vector2UInt textSize = _messageContent.textArea().computePreferredSize(measureWidth);
			minimum.y += static_cast<float>(textSize.y + 8);
		}
		setMinimumContentSize(minimum);
	}

	void MessageBox::setText(Font::Text text) { _messageContent.setMessage(std::move(text)); }
	void MessageBox::setText(std::string_view text) { _messageContent.setMessage(text); }
	const Font::Text &MessageBox::text() const noexcept { return _messageContent.message(); }
	void MessageBox::setMinimalWidth(unsigned int width)
	{
		if (_minimalWidth == width) return;
		_minimalWidth = width;
		_updateMinimumContentSize();
	}
	unsigned int MessageBox::minimalWidth() const noexcept { return _minimalWidth; }

	PushButton &MessageBox::addButton(std::string name, std::string_view label) { return _messageContent.addButton(std::move(name), label); }
	PushButton &MessageBox::button(std::string_view name) { return _messageContent.button(name); }
	const PushButton &MessageBox::button(std::string_view name) const { return _messageContent.button(name); }
	void MessageBox::removeButton(std::string_view name) { _messageContent.removeButton(name); }
	std::size_t MessageBox::nbButton() const noexcept { return _messageContent.nbButton(); }
	PushButton::ClickContract MessageBox::subscribe(std::string_view name, PushButton::ClickCallback callback)
	{
		return _messageContent.subscribe(name, std::move(callback));
	}

	MessageBox::Content &MessageBox::messageContent() noexcept { return _messageContent; }
	const MessageBox::Content &MessageBox::messageContent() const noexcept { return _messageContent; }
	TextArea &MessageBox::textArea() noexcept { return _messageContent.textArea(); }
	const TextArea &MessageBox::textArea() const noexcept { return _messageContent.textArea(); }
	CommandPanel &MessageBox::commandPanel() noexcept { return _messageContent.commandPanel(); }
	const CommandPanel &MessageBox::commandPanel() const noexcept { return _messageContent.commandPanel(); }

	InformationMessageBox::InformationMessageBox(std::string name, Widget *parent) : MessageBox(std::move(name), parent)
	{
		setTitle("Information");
		_closeButton = &addButton("close", "Close");
		_buttonContract = subscribe("close", [this]() { close(); });
	}

	PushButton &InformationMessageBox::closeButton() noexcept { return *_closeButton; }
	const PushButton &InformationMessageBox::closeButton() const noexcept { return *_closeButton; }

	RequestMessageBox::RequestMessageBox(std::string name, Widget *parent) : MessageBox(std::move(name), parent)
	{
		setTitle("Request");
		_firstButton = &addButton("first", "Yes");
		_secondButton = &addButton("second", "No");
		_firstContract = subscribe("first", [this]() {
			if (_firstAction) _firstAction();
			close();
		});
		_secondContract = subscribe("second", [this]() {
			if (_secondAction) _secondAction();
			close();
		});
		_titleCloseContract = subscribeToClose([this]() {
			const Action &action = _useSecondActionOnTitleClose ? _secondAction : _titleCloseAction;
			if (action) action();
		});
	}

	void RequestMessageBox::configure(
		std::string_view firstCaption,
		Action firstAction,
		std::string_view secondCaption,
		Action secondAction)
	{
		_firstButton->setText(firstCaption);
		_secondButton->setText(secondCaption);
		_firstAction = std::move(firstAction);
		_secondAction = std::move(secondAction);
	}

	void RequestMessageBox::setTitleCloseAction(Action action)
	{
		_titleCloseAction = std::move(action);
		_useSecondActionOnTitleClose = false;
	}

	void RequestMessageBox::resetTitleCloseAction()
	{
		_titleCloseAction = {};
		_useSecondActionOnTitleClose = true;
	}

	bool RequestMessageBox::usesSecondActionOnTitleClose() const noexcept
	{
		return _useSecondActionOnTitleClose;
	}

	PushButton &RequestMessageBox::firstButton() noexcept { return *_firstButton; }
	const PushButton &RequestMessageBox::firstButton() const noexcept { return *_firstButton; }
	PushButton &RequestMessageBox::secondButton() noexcept { return *_secondButton; }
	const PushButton &RequestMessageBox::secondButton() const noexcept { return *_secondButton; }
}
