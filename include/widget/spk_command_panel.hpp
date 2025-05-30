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

		void _onGeometryChange()
		{
			_layout.setGeometry({0, geometry().size});
		}

		void _composeLayout()
		{
			_layout.clear();
			
			if (_sizePolicy == spk::Layout::SizePolicy::Minimum || 
				_sizePolicy == spk::Layout::SizePolicy::VerticalExtend)
			{
				_layout.addWidget(&_spacer, spk::Layout::SizePolicy::Extend);
			}

			for (auto& tmpButton : _orderedButtons)
			{
				_layout.addWidget(tmpButton, _sizePolicy);
			}
		}

	public:
		CommandPanel(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent),
			_spacer(p_name + L"/SpacerWidget", this)
		{
			_layout.setElementPadding({10, 10});
			_spacer.activate();
			_composeLayout();
		}

		virtual spk::SafePointer<spk::PushButton> addButton(const std::wstring& p_name, const std::wstring& p_label)
		{
			if (_buttons.find(p_name) != _buttons.end())
			{
				throw std::runtime_error("Button with the same name already exists.");
			}

			std::unique_ptr<spk::PushButton>& newButton = _buttons.emplace(
				p_name, std::make_unique<spk::PushButton>(name() + L"/" + p_name, this)).first->second;
			
			newButton->setText(p_label);
			newButton->activate();

			_orderedButtons.push_back(newButton.get());

			_composeLayout();

			return (newButton.get());
		}

		spk::SafePointer<spk::PushButton> button(const std::wstring& p_name)
		{
			if (_buttons.contains(p_name) == false)
			{
				throw std::runtime_error("Button [" + spk::StringUtils::wstringToString(p_name) + "] doesn't exist in the command panel [" + spk::StringUtils::wstringToString(name()) + "]");
			}
			return (_buttons.at(p_name).get());
		}

		spk::SafePointer<const spk::PushButton> button(const std::wstring& p_name) const
		{
			if (_buttons.contains(p_name) == false)
			{
				throw std::runtime_error("Button [" + spk::StringUtils::wstringToString(p_name) + "] doesn't exist in the command panel [" + spk::StringUtils::wstringToString(name()) + "]");
			}
			return (_buttons.at(p_name).get());
		}

		void removeButton(const std::wstring& p_name)
		{
			auto it = _buttons.find(p_name);
			if (it != _buttons.end())
			{
				_layout.removeWidget(it->second.get());
				_buttons.erase(it);
			}
		}

		Contract subscribe(const std::wstring& p_name, const Job& p_job)
		{
			auto it = _buttons.find(p_name);
			if (it == _buttons.end())
			{
				throw std::runtime_error("Button not found.");
			}
			return it->second->subscribe(p_job);
		}

		spk::Vector2UInt minimalSize() const override
		{
			spk::Vector2UInt result = { 0, 0 };
			
			if (_buttons.size() != 0)
			{
				spk::Vector2UInt padding = _layout.elementPadding();

				for (const auto& [name, button] : _buttons)
				{
					spk::Vector2UInt buttonSize = button->minimalSize();
					result.x += buttonSize.x;
					result.y = std::max(result.y, buttonSize.y);
				}

				result.x += padding.x * (_buttons.size() - 1);
			}

			return result;
		}

		void setSizePolicy(const spk::Layout::SizePolicy &p_sizePolicy)
		{
			_sizePolicy = p_sizePolicy;
			_composeLayout();
		}

		const spk::Layout::SizePolicy &sizePolicy() const
		{
			return _sizePolicy;
		}
	};
}
