#include "ui/widget/command_panel.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
	constexpr float Unlimited = static_cast<float>(std::numeric_limits<std::size_t>::max());
}

namespace spk
{
	CommandPanel::CommandPanel(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_spacer(this->name() + ".spacer", this)
	{
		_spacer.setMinimalSize({0.0f, 0.0f});
		_spacer.setPreferredSize({0.0f, 0.0f});
		_spacer.setMaximalSize({Unlimited, Unlimited});
		setMaximalSize({Unlimited, Unlimited});
		_layoutReady = true;
		_rebuildLayout();
		activate();
	}

	void CommandPanel::_rebuildLayout()
	{
		if (!_layoutReady)
		{
			return;
		}

		_layout.clear();
		if (_sizePolicy.horizontal != Layout::SizePolicy::Extend)
		{
			_layout.addWidget(&_spacer, {Layout::SizePolicy::Extend, Layout::SizePolicy::Minimum});
		}
		for (const std::string &name : _buttonOrder)
		{
			Layout::Element *element = _layout.addWidget(_buttons.at(name).get(), _sizePolicy);
			element->setVerticalAlignment(VerticalAlignment::Center);
		}
		_updateSizeHint();
		_onGeometryChange();
	}

	void CommandPanel::_updateSizeHint()
	{
		if (!_layoutReady)
		{
			return;
		}

		Vector2 minimal{};
		Vector2 preferred{};
		for (const std::string &name : _buttonOrder)
		{
			const PushButton &current = *_buttons.at(name);
			minimal.x += current.minimalSize().x;
			minimal.y = std::max(minimal.y, current.minimalSize().y);
			preferred.x += current.preferredSize().x;
			preferred.y = std::max(preferred.y, current.preferredSize().y);
		}

		if (_buttonOrder.size() > 1)
		{
			const float padding = static_cast<float>(_buttonOrder.size() - 1) * static_cast<float>(_layout.elementPadding().x);
			minimal.x += padding;
			preferred.x += padding;
		}
		SizeHint hint = sizeHint();
		hint.minimal = minimal;
		hint.preferred = preferred;
		setSizeHint(hint);
		_onGeometryChange();
	}

	void CommandPanel::_onGeometryChange()
	{
		if (!_layoutReady)
		{
			return;
		}
		_layout.setGeometry(Rect2D{Vector2Int{0, 0}, geometry().size});
	}

	PushButton &CommandPanel::addButton(std::string name, std::string_view label)
	{
		if (_buttons.contains(name))
		{
			throw std::invalid_argument("CommandPanel already contains a button named '" + name + "'");
		}

		auto newButton = std::make_unique<PushButton>(this->name() + ".button-" + name, this);
		newButton->setMaximalSize({Unlimited, Unlimited});
		newButton->setText(label);
		PushButton &result = *newButton;
		_buttonOrder.push_back(name);
		_buttons.emplace(std::move(name), std::move(newButton));
		_rebuildLayout();
		return result;
	}

	PushButton &CommandPanel::button(std::string_view name)
	{
		const auto it = _buttons.find(std::string(name));
		if (it == _buttons.end())
		{
			throw std::out_of_range("CommandPanel has no button named '" + std::string(name) + "'");
		}
		return *it->second;
	}

	const PushButton &CommandPanel::button(std::string_view name) const
	{
		const auto it = _buttons.find(std::string(name));
		if (it == _buttons.end())
		{
			throw std::out_of_range("CommandPanel has no button named '" + std::string(name) + "'");
		}
		return *it->second;
	}

	void CommandPanel::removeButton(std::string_view name)
	{
		const std::string key(name);
		const auto it = _buttons.find(key);
		if (it == _buttons.end())
		{
			return;
		}

		_layout.removeWidget(it->second.get());
		_buttonOrder.erase(std::remove(_buttonOrder.begin(), _buttonOrder.end(), key), _buttonOrder.end());
		_buttons.erase(it);
		_rebuildLayout();
	}

	std::size_t CommandPanel::nbButton() const noexcept
	{
		return _buttonOrder.size();
	}

	PushButton::ClickContract CommandPanel::subscribe(std::string_view name, PushButton::ClickCallback callback)
	{
		return button(name).subscribeToClick(std::move(callback));
	}

	void CommandPanel::setSizePolicy(Layout::SizeSettings sizePolicy)
	{
		if (_sizePolicy == sizePolicy)
		{
			return;
		}
		_sizePolicy = sizePolicy;
		_rebuildLayout();
	}

	const Layout::SizeSettings &CommandPanel::sizePolicy() const noexcept
	{
		return _sizePolicy;
	}

	void CommandPanel::setElementPadding(const Vector2UInt &padding)
	{
		if (_layout.elementPadding() == padding)
		{
			return;
		}
		_layout.setElementPadding(padding);
		_updateSizeHint();
		_onGeometryChange();
	}

	const Vector2UInt &CommandPanel::elementPadding() const noexcept
	{
		return _layout.elementPadding();
	}
}
