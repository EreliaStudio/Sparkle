#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "ui/widget/interface_window.hpp"
#include "ui/widget/prompt_panel.hpp"

// WinUser.h exposes MessageBox as a MessageBoxA/MessageBoxW selection macro.
// It must not rewrite the public spk::MessageBox C++ type name.
#ifdef MessageBox
#	undef MessageBox
#endif

namespace spk
{
	class MessageBox : public IInterfaceWindow
	{
	public:
		class Content final : public PromptPanel
		{
		public:
			explicit Content(std::string name, Widget *parent = nullptr) :
				PromptPanel(std::move(name), parent)
			{
			}
		};

	private:
		Content _messageContent;
		unsigned int _minimalWidth = 200;
		CloseContract _closeContract;
		ResizeContract _resizeContract;
		ResizeableTrait::Contract _contentSizeContract;

		void _updateMinimumContentSize();

	public:
		explicit MessageBox(std::string name, Widget *parent = nullptr);

		void setText(Font::Text text);
		void setText(std::string_view text);
		[[nodiscard]] const Font::Text &text() const noexcept;
		void setMinimalWidth(unsigned int width);
		[[nodiscard]] unsigned int minimalWidth() const noexcept;

		PushButton &addButton(std::string name, std::string_view label);
		PushButton &button(std::string_view name);
		[[nodiscard]] const PushButton &button(std::string_view name) const;
		void removeButton(std::string_view name);
		[[nodiscard]] std::size_t nbButton() const noexcept;
		[[nodiscard]] PushButton::ClickContract subscribe(std::string_view name, PushButton::ClickCallback callback);

		[[nodiscard]] Content &messageContent() noexcept;
		[[nodiscard]] const Content &messageContent() const noexcept;
		[[nodiscard]] TextArea &textArea() noexcept;
		[[nodiscard]] const TextArea &textArea() const noexcept;
		[[nodiscard]] CommandPanel &commandPanel() noexcept;
		[[nodiscard]] const CommandPanel &commandPanel() const noexcept;
	};

	class InformationMessageBox : public MessageBox
	{
	private:
		PushButton *_closeButton = nullptr;
		PushButton::ClickContract _buttonContract;

	public:
		explicit InformationMessageBox(std::string name, Widget *parent = nullptr);
		[[nodiscard]] PushButton &closeButton() noexcept;
		[[nodiscard]] const PushButton &closeButton() const noexcept;
	};

	class RequestMessageBox : public MessageBox
	{
	public:
		using Action = std::function<void()>;

	private:
		PushButton *_firstButton = nullptr;
		PushButton *_secondButton = nullptr;
		Action _firstAction;
		Action _secondAction;
		Action _titleCloseAction;
		bool _useSecondActionOnTitleClose = true;
		PushButton::ClickContract _firstContract;
		PushButton::ClickContract _secondContract;
		CloseContract _titleCloseContract;

	public:
		explicit RequestMessageBox(std::string name, Widget *parent = nullptr);
		void configure(std::string_view firstCaption, Action firstAction, std::string_view secondCaption, Action secondAction);
		void setTitleCloseAction(Action action);
		void resetTitleCloseAction();
		[[nodiscard]] bool usesSecondActionOnTitleClose() const noexcept;
		[[nodiscard]] PushButton &firstButton() noexcept;
		[[nodiscard]] const PushButton &firstButton() const noexcept;
		[[nodiscard]] PushButton &secondButton() noexcept;
		[[nodiscard]] const PushButton &secondButton() const noexcept;
	};
}
