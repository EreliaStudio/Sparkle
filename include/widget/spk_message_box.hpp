// ────────────────────────────────────────────────────────────────
//  message_box.hpp
// ────────────────────────────────────────────────────────────────
#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>

#include "widget/spk_interface_window.hpp"   // IInterfaceWindow
#include "widget/spk_text_area.hpp"          // TextArea
#include "widget/spk_command_panel.hpp"      // CommandPanel
#include "widget/spk_linear_layout.hpp"

#ifdef MessageBox
#undef MessageBox
#endif

namespace spk
{
	class MessageBox : public spk::IInterfaceWindow
	{
	public:
		class Content : public spk::Widget
		{
		private:
			spk::VerticalLayout _layout;
			spk::TextArea       _textArea;
			spk::CommandPanel   _commandPanel;

			void _onGeometryChange() override;
			void _compose();

		public:
			Content(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

			const spk::VerticalLayout& layout() const;
			const spk::TextArea& textArea() const;
			const spk::CommandPanel& commandPanel() const;

			spk::VerticalLayout& layout();
			spk::TextArea& textArea();
			spk::CommandPanel& commandPanel();

			spk::Vector2UInt minimalSize() const override;
		};

	private:
		Content _content;

		spk::IInterfaceWindow::ResizeContractProvider::Contract _onResizeContract;

	public:
		MessageBox(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);

		const Content& content() const;
		const spk::TextArea& textArea() const;
		const spk::CommandPanel& commandPanel() const;

		Content& content();
		spk::TextArea& textArea();
		spk::CommandPanel& commandPanel();

		void setTitle(const std::wstring& p_title);

		virtual void setText(const std::wstring& p_text);

		const std::wstring& text() const;

		spk::SafePointer<spk::PushButton> addButton(const std::wstring& p_name, const std::wstring& p_label);
		spk::SafePointer<const spk::PushButton> button(const std::wstring& p_name) const;
		spk::SafePointer<spk::PushButton> button(const std::wstring& p_name);
		void removeButton(const std::wstring& p_name);

		CommandPanel::Contract subscribe(const std::wstring& p_name, const CommandPanel::Job& p_job);

		void setMinimalWidth(uint32_t p_width);
	};

	class InformationMessageBox : public MessageBox
	{
	private:
		spk::PushButton::Contract _closeContract;
		spk::SafePointer<spk::PushButton> _button;
        spk::PushButton::Contract _contract;

	public:
		InformationMessageBox(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);
		spk::SafePointer<spk::PushButton> button() const;
	};

	class RequestMessageBox : public MessageBox
	{
	private:
		spk::PushButton::Contract _closeContract;

		spk::SafePointer<spk::PushButton> _firstButton;
		spk::PushButton::Contract _firstContract;

		spk::SafePointer<spk::PushButton> _secondButton;
		spk::PushButton::Contract _secondContract;

	public:
		RequestMessageBox(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

		void configure(const std::wstring& p_firstCaption, const std::function<void()>& p_firstAction,
					const std::wstring& p_secondCaption, const std::function<void()>& p_secondAction);

		spk::SafePointer<spk::PushButton> firstButton() const;
		spk::SafePointer<spk::PushButton> secondButton() const;
	};
}
