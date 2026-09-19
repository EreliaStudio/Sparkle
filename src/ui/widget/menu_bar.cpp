#include "ui/widget/menu_bar.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

#include "rendering/command/sprite_render_command.hpp"

namespace
{
	constexpr float Unlimited = static_cast<float>(std::numeric_limits<std::size_t>::max());

	unsigned int dimension(float value) noexcept
	{
		return value <= 0.0f ? 0u : static_cast<unsigned int>(std::ceil(value));
	}

	unsigned int reduced(unsigned int value, unsigned int amount) noexcept
	{
		return value > amount ? value - amount : 0;
	}
}

namespace spk
{
	MenuBar::Menu::Item::Item(std::string name, std::string_view label, ClickCallback callback, Menu &owner) :
		PushButton(std::move(name), &owner),
		_owner(owner)
	{
		setText(label);
		setFlat(true);
		setAlignment({Alignment::Horizontal::Left, Alignment::Vertical::Center});
		_actionContract = subscribeToClick([this, callback = std::move(callback)]() {
			if (callback)
			{
				callback();
			}
			_owner.deactivate();
		});
	}

	MenuBar::Menu::Break::Break(std::string name, const SpriteSheet *spriteSheet, Widget *parent) :
		Widget(std::move(name), parent)
	{
		setSpriteSheet(spriteSheet);
		activate();
	}

	MenuBar::Menu::Break::Break(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		applyStyle(defaultStyle);
		activate();
	}

	void MenuBar::Menu::Break::applyStyle(const Style &style)
	{
		if (style.menuBreak != nullptr)
		{
			setSpriteSheet(style.menuBreak.get());
		}
	}

	void MenuBar::Menu::Break::_updateSizeHint()
	{
		SizeHint hint = sizeHint();
		hint.minimal = {1.0f, static_cast<float>(_height)};
		hint.preferred = hint.minimal;
		hint.maximal = {Unlimited, static_cast<float>(_height)};
		setSizeHint(hint);
	}

	void MenuBar::Menu::Break::_buildRenderSnapshot(RenderSnapshot::Builder &builder)
	{
		if (_spriteSheet == nullptr || geometry().width == 0 || geometry().height == 0)
		{
			return;
		}
		const unsigned int edge = std::min(_spriteSheet->size().x / 3, geometry().width / 2);
		auto &pass = builder.renderPass(targetRenderPass());
		pass.emplace<SpriteRenderCommand>(
			_spriteSheet,
			Vector2UInt{0, 0},
			Rect2D{Vector2Int{0, 0}, Vector2UInt{edge, geometry().height}},
			_depth);
		pass.emplace<SpriteRenderCommand>(
			_spriteSheet,
			Vector2UInt{1, 0},
			Rect2D{
				Vector2Int{static_cast<int>(edge), 0},
				Vector2UInt{geometry().width - 2 * edge, geometry().height}},
			_depth);
		pass.emplace<SpriteRenderCommand>(
			_spriteSheet,
			Vector2UInt{2, 0},
			Rect2D{
				Vector2Int{static_cast<int>(geometry().width - edge), 0},
				Vector2UInt{edge, geometry().height}},
			_depth);
	}

	void MenuBar::Menu::Break::setSpriteSheet(const SpriteSheet *spriteSheet)
	{
		if (spriteSheet == nullptr)
		{
			throw std::invalid_argument("Menu separator sprite sheet cannot be null");
		}
		if (spriteSheet->nbSprite() != Vector2UInt{3, 1})
		{
			throw std::invalid_argument("Menu separator requires a 3x1 sprite sheet");
		}
		_spriteSheet = spriteSheet;
		_updateSizeHint();
	}

	void MenuBar::Menu::Break::setHeight(unsigned int height)
	{
		if (_height == height)
		{
			return;
		}
		_height = height;
		_updateSizeHint();
	}

	void MenuBar::Menu::Break::setDepth(float depth)
	{
		_depth = depth;
	}

	const SpriteSheet *MenuBar::Menu::Break::spriteSheet() const noexcept
	{
		return _spriteSheet;
	}
	unsigned int MenuBar::Menu::Break::height() const noexcept
	{
		return _height;
	}
	float MenuBar::Menu::Break::depth() const noexcept
	{
		return _depth;
	}

	MenuBar::Menu::Menu(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_background(this->name() + ".background", this)
	{
		applyStyle(defaultStyle);
		_background.setZOrder(0.0f);
		setMaximalSize({Unlimited, Unlimited});
		_layoutReady = true;
		_updateSizeHint();
		deactivate();
	}

	void MenuBar::Menu::applyStyle(const Style &style)
	{
		if (style.darkNineSlice != nullptr)
		{
			_background.setSpriteSheet(style.darkNineSlice.get());
		}
		_background.setCornerSize({10, 10});
		for (const auto &element : _elements)
		{
			element->applyStyle(style);
		}
	}

	void MenuBar::Menu::_updateSizeHint()
	{
		if (!_layoutReady)
		{
			return;
		}
		Vector2 content{};
		for (const auto &element : _elements)
		{
			content.x = std::max(content.x, element->minimalSize().x);
			content.y += element->minimalSize().y;
		}
		if (_elements.size() > 1)
		{
			content.y += static_cast<float>((_elements.size() - 1) * _elementSpacing);
		}
		const Vector2Int corner = _background.cornerSize();
		content.x += static_cast<float>(2 * (std::max(corner.x, 0) + static_cast<int>(_elementSpacing)));
		content.y += static_cast<float>(2 * std::max(corner.y, 0));
		SizeHint hint = sizeHint();
		hint.minimal = content;
		hint.preferred = content;
		setSizeHint(hint);
		_onGeometryChange();
	}

	void MenuBar::Menu::_onGeometryChange()
	{
		if (!_layoutReady)
		{
			return;
		}
		_background.setGeometry(Rect2D{Vector2Int{0, 0}, geometry().size});
		const Vector2Int corner = _background.cornerSize();
		const unsigned int horizontalMargin = std::min(
			static_cast<unsigned int>(std::max(corner.x, 0)) + _elementSpacing,
			geometry().width / 2);
		const unsigned int verticalMargin = std::min(
			static_cast<unsigned int>(std::max(corner.y, 0)),
			geometry().height / 2);
		int y = static_cast<int>(verticalMargin);
		const unsigned int width = reduced(geometry().width, 2 * horizontalMargin);
		for (const auto &element : _elements)
		{
			const unsigned int height = dimension(element->minimalSize().y);
			element->setGeometry(Rect2D{Vector2Int{static_cast<int>(horizontalMargin), y}, Vector2UInt{width, height}});
			y += static_cast<int>(height + _elementSpacing);
		}
	}

	void MenuBar::Menu::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (!viewRegion().viewport.contains(event.device.position))
		{
			deactivate();
		}
	}

	MenuBar::Menu::Item &MenuBar::Menu::addItem(std::string name, std::string_view label, PushButton::ClickCallback callback)
	{
		auto item = std::make_unique<Item>(this->name() + ".item-" + name, label, std::move(callback), *this);
		item->setZOrder(1.0f);
		Item &result = *item;
		_elements.push_back(std::move(item));
		_updateSizeHint();
		return result;
	}

	MenuBar::Menu::Break &MenuBar::Menu::addBreak(std::string name, const SpriteSheet *spriteSheet)
	{
		auto separator = std::make_unique<Break>(this->name() + ".break-" + name, spriteSheet, this);
		separator->setZOrder(1.0f);
		Break &result = *separator;
		_elements.push_back(std::move(separator));
		_updateSizeHint();
		return result;
	}

	MenuBar::Menu::Break &MenuBar::Menu::addBreak(std::string name)
	{
		auto separator = std::make_unique<Break>(this->name() + ".break-" + name, this);
		separator->setZOrder(1.0f);
		Break &result = *separator;
		_elements.push_back(std::move(separator));
		_updateSizeHint();
		return result;
	}

	void MenuBar::Menu::clear()
	{
		_elements.clear();
		deactivate();
		_updateSizeHint();
	}

	std::size_t MenuBar::Menu::nbElement() const noexcept
	{
		return _elements.size();
	}
	void MenuBar::Menu::setElementSpacing(unsigned int spacing)
	{
		if (_elementSpacing == spacing)
		{
			return;
		}
		_elementSpacing = spacing;
		_updateSizeHint();
	}
	unsigned int MenuBar::Menu::elementSpacing() const noexcept
	{
		return _elementSpacing;
	}
	Panel &MenuBar::Menu::background() noexcept
	{
		return _background;
	}
	const Panel &MenuBar::Menu::background() const noexcept
	{
		return _background;
	}

	MenuBar::MenuBar(std::string name, Widget *parent) :
		Widget(std::move(name), parent),
		_background(this->name() + ".background", this)
	{
		applyStyle(defaultStyle);
		_background.setZOrder(0.0f);
		setMaximalSize({Unlimited, Unlimited});
		_layoutReady = true;
		_updateSizeHint();
		activate();
	}

	void MenuBar::applyStyle(const Style &style)
	{
		if (style.darkerNineSlice != nullptr)
		{
			_background.setSpriteSheet(style.darkerNineSlice.get());
		}
		_background.setCornerSize({6, 6});
		for (const auto &entry : _entries)
		{
			entry->button->applyStyle(style);
			entry->menu->applyStyle(style);
		}
	}

	unsigned int MenuBar::_effectiveHeight() const noexcept
	{
		unsigned int result = _height;
		for (const auto &entry : _entries)
		{
			result = std::max(result, dimension(entry->button->minimalSize().y) + 2 * _contentInset);
		}
		return result;
	}

	void MenuBar::_closeMenus(Menu *exception)
	{
		for (const auto &entry : _entries)
		{
			if (entry->menu.get() != exception)
			{
				entry->menu->deactivate();
			}
		}
	}

	void MenuBar::_configureButton(PushButton &button)
	{
		button.setFlat(true);
		button.setAlignment({Alignment::Horizontal::Center, Alignment::Vertical::Center});
		const unsigned int controlHeight = _effectiveHeight();
		const Font::Size textSize{std::max<std::size_t>(8, controlHeight / 2)};
		for (TextLabel *label : {&button.releasedLabel(), &button.pressedLabel()})
		{
			label->setTextSize(textSize);
		}
		button.setTextPadding({std::max(4u, controlHeight / 4), std::max(1u, controlHeight / 12)});
	}

	void MenuBar::_updateSizeHint()
	{
		if (!_layoutReady)
		{
			return;
		}
		float width = static_cast<float>(2 * _contentInset);
		for (const auto &entry : _entries)
		{
			width += entry->button->minimalSize().x;
		}
		if (_entries.size() > 1)
		{
			width += static_cast<float>((_entries.size() - 1) * _menuSpacing);
		}
		SizeHint hint = sizeHint();
		hint.minimal = {width, static_cast<float>(_effectiveHeight())};
		hint.preferred = hint.minimal;
		setSizeHint(hint);
		_onGeometryChange();
	}

	void MenuBar::_onGeometryChange()
	{
		if (!_layoutReady)
		{
			return;
		}
		const unsigned int barHeight = std::min(_effectiveHeight(), geometry().height);
		_background.setGeometry(Rect2D{Vector2Int{0, 0}, Vector2UInt{geometry().width, barHeight}});
		int x = static_cast<int>(_contentInset);
		for (const auto &entry : _entries)
		{
			const unsigned int width = dimension(entry->button->minimalSize().x);
			entry->button->setGeometry(Rect2D{Vector2Int{x, static_cast<int>(_contentInset)}, Vector2UInt{width, reduced(barHeight, 2 * _contentInset)}});
			entry->menu->setGeometry(Rect2D{Vector2Int{x, static_cast<int>(barHeight)}, Vector2UInt{dimension(entry->menu->minimalSize().x), dimension(entry->menu->minimalSize().y)}});
			x += static_cast<int>(width + _menuSpacing);
		}
	}

	MenuBar::Menu &MenuBar::addMenu(std::string name, std::string_view label)
	{
		if (_entriesByName.contains(name))
		{
			throw std::invalid_argument("MenuBar already contains a menu named '" + name + "'");
		}
		auto entry = std::make_unique<Entry>();
		entry->button = std::make_unique<PushButton>(this->name() + ".button-" + name, this);
		entry->menu = std::make_unique<Menu>(this->name() + ".menu-" + name, this);
		entry->button->setZOrder(1.0f);
		entry->menu->setZOrder(2.0f);
		entry->button->setText(label.empty() ? std::string_view(name) : label);
		_configureButton(*entry->button);
		Entry *entryPointer = entry.get();
		entry->toggleContract = entry->button->subscribeToClick([this, entryPointer]() {
			const bool wasOpen = entryPointer->menu->isActive();
			_closeMenus();
			if (!wasOpen && entryPointer->menu->nbElement() != 0)
			{
				entryPointer->menu->activate();
			}
			_onGeometryChange();
		});
		Menu &result = *entry->menu;
		_entriesByName.emplace(name, entryPointer);
		_entries.push_back(std::move(entry));
		_updateSizeHint();
		return result;
	}

	MenuBar::Menu &MenuBar::menu(std::string_view name)
	{
		const auto it = _entriesByName.find(std::string(name));
		if (it == _entriesByName.end())
		{
			throw std::out_of_range("MenuBar has no menu named '" + std::string(name) + "'");
		}
		return *it->second->menu;
	}
	const MenuBar::Menu &MenuBar::menu(std::string_view name) const
	{
		const auto it = _entriesByName.find(std::string(name));
		if (it == _entriesByName.end())
		{
			throw std::out_of_range("MenuBar has no menu named '" + std::string(name) + "'");
		}
		return *it->second->menu;
	}
	PushButton &MenuBar::button(std::string_view name)
	{
		const auto it = _entriesByName.find(std::string(name));
		if (it == _entriesByName.end())
		{
			throw std::out_of_range("MenuBar has no menu named '" + std::string(name) + "'");
		}
		return *it->second->button;
	}
	const PushButton &MenuBar::button(std::string_view name) const
	{
		const auto it = _entriesByName.find(std::string(name));
		if (it == _entriesByName.end())
		{
			throw std::out_of_range("MenuBar has no menu named '" + std::string(name) + "'");
		}
		return *it->second->button;
	}
	std::size_t MenuBar::nbMenu() const noexcept
	{
		return _entries.size();
	}
	void MenuBar::closeMenus()
	{
		_closeMenus();
	}
	void MenuBar::setHeight(unsigned int height)
	{
		if (_height == height)
		{
			return;
		}
		_height = height;
		for (const auto &entry : _entries)
		{
			_configureButton(*entry->button);
		}
		_updateSizeHint();
	}
	unsigned int MenuBar::height() const noexcept
	{
		return _effectiveHeight();
	}
	void MenuBar::setContentInset(unsigned int inset)
	{
		if (_contentInset == inset)
		{
			return;
		}
		_contentInset = inset;
		_updateSizeHint();
	}
	unsigned int MenuBar::contentInset() const noexcept
	{
		return _contentInset;
	}
	void MenuBar::setMenuSpacing(unsigned int spacing)
	{
		if (_menuSpacing == spacing)
		{
			return;
		}
		_menuSpacing = spacing;
		_updateSizeHint();
	}
	unsigned int MenuBar::menuSpacing() const noexcept
	{
		return _menuSpacing;
	}
	Panel &MenuBar::background() noexcept
	{
		return _background;
	}
	const Panel &MenuBar::background() const noexcept
	{
		return _background;
	}
}
