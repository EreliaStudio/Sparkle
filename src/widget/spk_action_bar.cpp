#include "widget/spk_action_bar.hpp"

#include "spk_generated_resources.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	MenuBar::Menu::Item::Item(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::PushButton(p_name, p_parent)
	{
		setNineSlice(nullptr, spk::PushButton::State::Pressed);
		setNineSlice(nullptr, spk::PushButton::State::Released);
		setCornerSize(0);
		setTextAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
	}

	spk::SpriteSheet MenuBar::Menu::Break::_defaultBreakSpriteSheet = spk::SpriteSheet::fromRawData(
		SPARKLE_GET_RESOURCE("resources/textures/defaultBreak.png"), spk::Vector2UInt(3, 1), SpriteSheet::Filtering::Nearest);

	spk::SafePointer<const spk::SpriteSheet> MenuBar::Menu::Break::defaultBreakSpriteSheet()
	{
		return (&_defaultBreakSpriteSheet);
	}

	void MenuBar::Menu::Break::_onGeometryChange()
	{
		_renderer.clear();

		_renderer.prepare(
			{spk::Vector2Int(0, 0), {_height, _height}}, _renderer.texture().upCast<spk::SpriteSheet>()->sprite(0), layer());

		_renderer.prepare({spk::Vector2Int(_height, 0), {geometry().size.x - _height * 2, _height}},
						  _renderer.texture().upCast<spk::SpriteSheet>()->sprite(1),
						  layer());

		_renderer.prepare({spk::Vector2Int(geometry().size.x - _height, 0), {_height, _height}},
						  _renderer.texture().upCast<spk::SpriteSheet>()->sprite(2),
						  layer());

		_renderer.validate();
	}

	void MenuBar::Menu::Break::_onPaintEvent(spk::PaintEvent &p_event)
	{
		_renderer.render();
	}

	MenuBar::Menu::Break::Break(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		setSpriteSheet(defaultBreakSpriteSheet());
	}

	spk::Vector2UInt MenuBar::Menu::Break::minimalSize() const
	{
		return (spk::Vector2UInt(1, _height));
	}

	void MenuBar::Menu::Break::setSpriteSheet(const spk::SafePointer<const spk::SpriteSheet> &p_spriteSheet)
	{
		if (p_spriteSheet->nbSprite() != spk::Vector2UInt(3, 1))
		{
			GENERATE_ERROR("Invalid sprite sheet size. Break expects a 3x1 sprite sheet.");
		}

		_renderer.setTexture(p_spriteSheet);
	}

	void MenuBar::Menu::Break::setHeight(const uint32_t &p_height)
	{
		_height = p_height;

		parent()->parent()->requireGeometryUpdate();
	}

	spk::Font::Size MenuBar::Menu::_computeElementSize() const
	{
		return (spk::Font::Size(17, 0));
	}

	void MenuBar::Menu::_onGeometryChange()
	{
		_backgroundFrame.setGeometry({0, 0}, geometry().size);

		spk::Vector2Int anchor = _backgroundFrame.cornerSize();
		anchor.x += 5;
		float contentSize = geometry().size.x - _backgroundFrame.cornerSize().x * 2 - 10;

		for (const auto &element : _elements)
		{
			spk::Vector2UInt elementTextSize = element.item->minimalSize();

			element.item->setGeometry(anchor, {contentSize, elementTextSize.y});

			anchor.y += elementTextSize.y + 5;
		}
	}

	void MenuBar::Menu::_onMouseEvent(spk::MouseEvent &p_event)
	{
		if (p_event.type == spk::MouseEvent::Type::Press)
		{
			if (isPointed(p_event.mouse) == false)
			{
				deactivate();
			}
		}
	}

	MenuBar::Menu::Menu(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_backgroundFrame(p_name + L" - Background frame", this)
	{
		_backgroundFrame.setCornerSize(2);
		_backgroundFrame.activate();
	}

	size_t MenuBar::Menu::nbItem() const
	{
		return (_elements.size());
	}

	spk::SafePointer<MenuBar::Menu::Item> MenuBar::Menu::addItem(const std::wstring &p_itemName, std::function<void()> p_callback)
	{
		auto item = std::make_unique<Item>(name() + L" - " + p_itemName, this);

		spk::Font::Size fontSize = _computeElementSize();
		item->setFontSize(fontSize);
		item->setFontSize({fontSize.glyph - 4, 0}, spk::PushButton::State::Pressed);
		item->setText(p_itemName);
		item->setNineSlice(nullptr, spk::PushButton::State::Released);
		item->setNineSlice(nullptr, spk::PushButton::State::Pressed);

		spk::PushButton::Contract contract = item->subscribe(p_callback);

		item->activate();

		_elements.push_back({std::move(item), std::move(contract)});

		parent()->requireGeometryUpdate();

		return dynamic_cast<Item *>(_elements.back().item.get());
	}

	spk::SafePointer<MenuBar::Menu::Break> MenuBar::Menu::addBreak()
	{
		auto item = std::make_unique<Break>(name() + L" - Break", this);

		item->activate();

		_elements.push_back({std::move(item), spk::PushButton::Contract()});

		parent()->requireGeometryUpdate();

		return dynamic_cast<Break *>(_elements.back().item.get());
	}

	spk::Vector2UInt MenuBar::Menu::minimalSize() const
	{
		spk::Vector2UInt result = 0;

		for (const auto &element : _elements)
		{
			if (result.y != 0)
			{
				result.y += 5;
			}
			spk::Vector2UInt elementTextSize = element.item->minimalSize();

			result.x = std::max(result.x, elementTextSize.x + 10);
			result.y += elementTextSize.y;
		}

		return (result + _backgroundFrame.cornerSize() * 2);
	}

	spk::Font::Size MenuBar::_computeFontSize() const
	{
		return (spk::Font::Size(_height - _backgroundFrame.cornerSize().y * 2, 0));
	}

	void MenuBar::_onGeometryChange()
	{
		_backgroundFrame.setGeometry({0, 0}, {geometry().size.x, _height});

		spk::Vector2Int anchor = _backgroundFrame.cornerSize();

		for (auto &entry : _menus)
		{
			spk::Font::Size fontSize = _computeFontSize();
			entry->menuButton->setFontSize(fontSize);
			entry->menuButton->setFontSize({fontSize.glyph - 4, 0}, spk::PushButton::State::Pressed);

			spk::Vector2Int buttonTextSize = entry->menuButton->computeTextSize();

			spk::Vector2UInt buttonSize = {buttonTextSize.x + entry->menuButton->cornerSize().x * 2 + (_height - buttonTextSize.y),
										   _height - _backgroundFrame.cornerSize().y * 2};

			entry->menuButton->setGeometry({anchor, buttonSize});

			entry->Menu->setGeometry({anchor.x, _backgroundFrame.geometry().size.y}, entry->Menu->minimalSize());

			anchor.x += buttonSize.x + 5;
		}
	}

	MenuBar::MenuBar(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_backgroundFrame(p_name + L" - BackgroundFrame", this)
	{
		_backgroundFrame.setLayer(1);
		_backgroundFrame.setCornerSize(2);
		_backgroundFrame.activate();
	}

	void MenuBar::setHeight(const float &p_height)
	{
		_height = p_height;
		requireGeometryUpdate();
	}

	const float &MenuBar::height() const
	{
		return (_height);
	}

	spk::SafePointer<MenuBar::Menu> MenuBar::addMenu(const std::wstring &p_menuName)
	{
		std::unique_ptr<MenuEntry> newMenuEntry = std::make_unique<MenuEntry>();

		newMenuEntry->menuButton = std::make_unique<spk::PushButton>(name() + L" - " + p_menuName + L" Menu button", this);
		newMenuEntry->menuButton->setText(p_menuName);
		newMenuEntry->menuButton->setNineSlice(nullptr);
		newMenuEntry->menuButton->setCornerSize(0);
		newMenuEntry->menuButton->setLayer(10);
		newMenuEntry->menuButton->activate();

		newMenuEntry->Menu = std::make_unique<Menu>(name() + L" - " + p_menuName + L" DropDown", this);
		newMenuEntry->Menu->setLayer(20);

		MenuEntry *rawMenuEntryPtr = newMenuEntry.get();

		newMenuEntry->menuButtonContract = newMenuEntry->menuButton->subscribe(
			[this, rawMenuEntryPtr]()
			{
				for (auto &entry : _menus)
				{
					entry->Menu->deactivate();
				}
				if (rawMenuEntryPtr->Menu->nbItem() != 0)
				{
					rawMenuEntryPtr->Menu->activate();
				}
				requireGeometryUpdate();
			});

		_menus.push_back(std::move(newMenuEntry));

		requireGeometryUpdate();
		requestPaint();

		return _menus.back()->Menu.get();
	}
}