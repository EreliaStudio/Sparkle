#pragma once

#include "widget/spk_widget.hpp"
#include "widget/spk_push_button.hpp"
#include "widget/spk_linear_layout.hpp"
#include "widget/spk_spacer_widget.hpp"

#include <unordered_map>
#include <string>

namespace spk
{
	class CommandPanel : public spk::Widget
	{
	public:
		using Contract = spk::ContractProvider::Contract;
		using Job = spk::ContractProvider::Job;

	private:
		spk::HorizontalLayout _layout;
		spk::SpacerWidget _spacer;
		spk::Layout::SizePolicy _sizePolicy = spk::Layout::SizePolicy::Minimum;
		std::unordered_map<std::wstring, std::unique_ptr<spk::PushButton>> _buttons;
		std::vector<spk::SafePointer<spk::PushButton>> _orderedButtons;

		void _onGeometryChange();
		void _composeLayout();

	public:
		CommandPanel(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

		virtual spk::SafePointer<spk::PushButton> addButton(const std::wstring& p_name, const std::wstring& p_label);
		spk::SafePointer<spk::PushButton> button(const std::wstring& p_name);
		spk::SafePointer<const spk::PushButton> button(const std::wstring& p_name) const;
		void removeButton(const std::wstring& p_name);

		Contract subscribe(const std::wstring& p_name, const Job& p_job);

		spk::Vector2UInt minimalSize() const override;

		void setSizePolicy(const spk::Layout::SizePolicy &p_sizePolicy);
		const spk::Layout::SizePolicy &sizePolicy() const;
	};
}
