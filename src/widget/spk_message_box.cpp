#include "widget/spk_message_box.hpp"

namespace spk
{
	void MessageBox::Content::_onGeometryChange()
	{
		_compose();
		_layout.setGeometry({0, geometry().size});
	}

	void MessageBox::Content::_compose()
	{
		_layout.clear();
		_layout.addWidget(&_textArea,     spk::Layout::SizePolicy::Extend);
		_layout.addWidget(&_commandPanel, spk::Layout::SizePolicy::Minimum);
	}

	MessageBox::Content::Content(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_textArea(p_name + L"/TextArea", this),
		_commandPanel(p_name + L"/CommandPanel", this)
	{
		_layout.setElementPadding({8, 8});

		_textArea.backgroundFrame().setCornerSize(0);
		_textArea.backgroundFrame().setNineSlice(nullptr);

		_textArea.activate();
		_commandPanel.activate();

		_compose();
	}

	const spk::VerticalLayout &MessageBox::Content::layout() const
	{
		return (_layout);
	}
	const spk::TextArea &MessageBox::Content::textArea() const
	{
		return (_textArea);
	}
	const spk::CommandPanel &MessageBox::Content::commandPanel() const
	{
		return (_commandPanel);
	}

	spk::VerticalLayout &MessageBox::Content::layout()
	{
		return (_layout);
	}
	spk::TextArea &MessageBox::Content::textArea()
	{
		return (_textArea);
	}
	spk::CommandPanel &MessageBox::Content::commandPanel()
	{
		return (_commandPanel);
	}

	spk::Vector2UInt MessageBox::Content::minimalSize() const
	{
		if (_textArea.text().empty())
		{
			return _commandPanel.minimalSize();
		}

		return (
			spk::Vector2UInt(
				std::max(_textArea.minimalSize().x, _commandPanel.minimalSize().x),
				_textArea.minimalSize().y + _layout.elementPadding().y + _commandPanel.minimalSize().y
			)
		);
	}

	MessageBox::MessageBox(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		spk::IInterfaceWindow(p_name, p_parent),
		_content(p_name + L"/Content", backgroundFrame())
	{
		setContent(&_content);
		_content.activate();

		_closeContract = subscribeTo(spk::IInterfaceWindow::Event::Close, [&]{ close(); });

		_onResizeContract = subscribeOnResize([&](const spk::Vector2UInt& p_availableSize){
		
			spk::Vector2UInt commandPanelMinimalSize = _content.commandPanel().minimalSize();
			size_t textAreaWidth = (commandPanelMinimalSize.x > p_availableSize.x ? commandPanelMinimalSize.x : p_availableSize.x);
			spk::Vector2UInt textAreaMinimalSize = _content.textArea().computeMinimalSize(textAreaWidth);

			spk::Vector2UInt minimalContentSize = spk::Vector2UInt(
					std::max(textAreaMinimalSize.x, commandPanelMinimalSize.x),
					textAreaMinimalSize.y + _content.layout().elementPadding().y + commandPanelMinimalSize.y
				);

			setMinimumContentSize(minimalContentSize);
			requireGeometryUpdate();
		});
	}

	const MessageBox::Content &MessageBox::content() const
	{
		return _content;
	}
	const spk::TextArea &MessageBox::textArea() const
	{
		return _content.textArea();
	}
	const spk::CommandPanel &MessageBox::commandPanel() const
	{
		return _content.commandPanel();
	}

	MessageBox::Content &MessageBox::content()
	{
		return _content;
	}
	spk::TextArea &MessageBox::textArea()
	{
		return _content.textArea();
	}
	spk::CommandPanel &MessageBox::commandPanel()
	{
		return _content.commandPanel();
	}

	void MessageBox::setTitle(const std::wstring& p_title)
	{
		menuBar()->titleLabel()->setText(p_title);
	}

	void MessageBox::setText(const std::wstring& p_text)
	{
		_content.textArea().setText(p_text);
	}

	const std::wstring& MessageBox::text() const
	{
		return _content.textArea().text();
	}

	spk::SafePointer<spk::PushButton> MessageBox::addButton(const std::wstring& p_name,
												const std::wstring& p_label)
	{
		return _content.commandPanel().addButton(p_name, p_label);
	}

	spk::SafePointer<const spk::PushButton> MessageBox::button(const std::wstring& p_name) const
	{
		return (_content.commandPanel().button(p_name));
	}

	spk::SafePointer<spk::PushButton> MessageBox::button(const std::wstring& p_name)
	{
		return (_content.commandPanel().button(p_name));
	}

	void MessageBox::removeButton(const std::wstring& p_name)
	{
		_content.commandPanel().removeButton(p_name);
	}

	CommandPanel::Contract MessageBox::subscribe(const std::wstring& p_name, const CommandPanel::Job& p_job)
	{
		return (std::move(_content.commandPanel().subscribe(p_name, p_job)));
	}

	void MessageBox::setMinimalWidth(uint32_t p_width)
	{
		spk::Vector2UInt commandPanelMinimalSize = _content.commandPanel().minimalSize();
		spk::Vector2UInt textAreaMinimalSize = _content.textArea().computeMinimalSize((commandPanelMinimalSize.x > p_width ? commandPanelMinimalSize.x : p_width));

		setMinimumContentSize(spk::Vector2UInt(
				std::max(textAreaMinimalSize.x, commandPanelMinimalSize.x),
				textAreaMinimalSize.y + _content.layout().elementPadding().y + commandPanelMinimalSize.y
			));
	}

	InformationMessageBox::InformationMessageBox(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		MessageBox(p_name, p_parent)
	{
		setTitle(L"Information");
		_button = addButton(p_name + L"/CloseButton", L"Close");
		_contract = _button->subscribe([&]{ close(); });
		
		_closeContract = subscribeTo(spk::IInterfaceWindow::Event::Close, [&]{ 
			close();
		});
	}

	spk::SafePointer<spk::PushButton> InformationMessageBox::button() const
	{
		return _button;
	}

	RequestMessageBox::RequestMessageBox(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		MessageBox(p_name, p_parent)
	{
		setTitle(L"Request");

		_firstButton = addButton(p_name + L"/FirstButton",  L"FirstButton");
		_secondButton = addButton(p_name + L"/SecondButton", L"SecondButton");
		configure(L"Yes", [&]{ },
				L"No", [&]{ });
	}

	void RequestMessageBox::configure(const std::wstring& p_firstCaption, const std::function<void()>& p_firstAction,
				const std::wstring& p_secondCaption, const std::function<void()>& p_secondAction)
	{
		_firstButton->setName(name() + L"/" + p_firstCaption + L"Button");
		_firstButton->setText(p_firstCaption);
		_firstContract = _firstButton->subscribe(p_firstAction);

		_secondButton->setName(name() + L"/" + p_secondCaption + L"Button");
		_secondButton->setText(p_secondCaption);
		_secondContract = _secondButton->subscribe(p_secondAction);

		_closeContract = subscribeTo(spk::IInterfaceWindow::Event::Close, p_secondAction);
	}

	spk::SafePointer<spk::PushButton> RequestMessageBox::firstButton() const
	{
		return _firstButton;
	}

	spk::SafePointer<spk::PushButton> RequestMessageBox::secondButton() const
	{
		return _secondButton;
	}
}