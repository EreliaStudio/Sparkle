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
	private:
		spk::HorizontalLayout _layout;
		spk::SpacerWidget _spacer;
		std::unordered_map<std::wstring, std::unique_ptr<spk::PushButton>> _buttons;

		void _onGeometryChange()
		{
			_layout.setGeometry({0, geometry().size});
		}

	public:
		CommandPanel(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent),
			_spacer(p_name + L"/SpacerWidget", this)
		{
			_layout.setElementPadding({10, 10});
			_spacer.activate();
			_layout.addWidget(&_spacer, spk::Layout::SizePolicy::Extend);
		}

		spk::SafePointer<spk::PushButton> addButton(const std::wstring& p_name, const std::wstring& p_label)
		{
			if (_buttons.find(p_name) != _buttons.end())
			{
				throw std::runtime_error("Button with the same name already exists.");
			}

			std::unique_ptr<spk::PushButton>& newButton = _buttons.emplace(
				p_name, std::make_unique<spk::PushButton>(p_name, this)).first->second;
			
			newButton->setText(p_label);
			newButton->activate();
			_layout.addWidget(newButton.get(), spk::Layout::SizePolicy::Minimum);

			return (newButton.get());
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

		spk::PushButton::Contract subscribe(const std::wstring& p_name, const PushButton::Job& p_job)
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
			spk::Vector2UInt padding = _layout.elementPadding();
			spk::Vector2UInt result = { padding.x, 0 };

			for (const auto& [name, button] : _buttons)
			{
				spk::Vector2UInt buttonSize = button->minimalSize();
				result.x += buttonSize.x + padding.x;
				result.y = std::max(result.y, buttonSize.y);
			}

			return result;
		}
	};
}
