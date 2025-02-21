#include "playground.hpp"

class MenuBarItem : public spk::PushButton
{
private:

public:
    MenuBarItem(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::PushButton(p_name, p_parent)
    {
		setNineSlice(nullptr, spk::PushButton::State::Pressed);
		setNineSlice(nullptr, spk::PushButton::State::Released);
        setCornerSize(0);
    }
};

class VerticalMenu : public spk::Widget
{
private:
	struct Element
	{
		std::unique_ptr<spk::Widget> item;
		spk::PushButton::Contract itemContract;
	};

	bool _selected = false;
	spk::Frame _backgroundFrame;
    std::vector<Element> _elements;

	spk::Font::Size _computeElementSize() const
	{
		return (spk::Font::Size(17, 0));
	}

    void _onGeometryChange() override
	{
		_backgroundFrame.setGeometry({0, 0}, geometry().size);

		spk::Vector2Int anchor = _backgroundFrame.cornerSize();
		anchor.x += 5;
		float contentSize = geometry().size.x - _backgroundFrame.cornerSize().x * 2 - 10;

		for (const auto& element : _elements)
		{
			spk::Vector2UInt elementTextSize = element.item->minimalSize();

			element.item->setGeometry(anchor, {contentSize, elementTextSize.y});

			anchor.y += elementTextSize.y + 5;
		}
    }

	void _onMouseEvent(spk::MouseEvent& p_event)
	{
		if (p_event.type == spk::MouseEvent::Type::Press)
		{		
			if (isPointed(p_event.mouse) == false)
			{
				deactivate();
			}		
		}
	}

public:
    VerticalMenu(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_backgroundFrame(p_name + L" - Background frame", this)
    {
		_backgroundFrame.setCornerSize(2);
		_backgroundFrame.activate();
    }

	size_t nbItem() const
	{
		return (_elements.size());
	}

    void addItem(const std::wstring& p_itemName, std::function<void()> p_callback)
    {
        auto item = std::make_unique<MenuBarItem>(name() + L" - " + p_itemName, this);

		spk::Font::Size fontSize = _computeElementSize();
		item->setFontSize(fontSize);
		item->setFontSize({fontSize.text - 4, 0}, spk::PushButton::State::Pressed);
        item->setText(p_itemName);
		item->setNineSlice(nullptr, spk::PushButton::State::Released);
		item->setNineSlice(nullptr, spk::PushButton::State::Pressed);
		
		spk::PushButton::Contract contract = item->subscribe(p_callback);

        item->activate();

        _elements.push_back({std::move(item), std::move(contract)});

        parent()->requireGeometryUpdate();
    }

	spk::Vector2UInt minimalSize() const
	{
		spk::Vector2UInt result = 0;

		for (const auto& element : _elements)
		{
			if (result.y != 0)
				result.y += 5;
			spk::Vector2UInt elementTextSize = element.item->minimalSize();

			result.x = std::max(result.x, elementTextSize.x + 10);
			result.y += elementTextSize.y;
		}

		return (result + _backgroundFrame.cornerSize() * 2);
	}
};

class MenuBarWidget : public spk::Widget
{
private:
	float _height = 25;
    spk::Frame _backgroundFrame;

    struct MenuEntry {
        std::unique_ptr<spk::PushButton> menuButton;
		spk::PushButton::Contract menuButtonContract;
        std::unique_ptr<VerticalMenu> verticalMenu;
    };

    std::vector<std::unique_ptr<MenuEntry>> _menus;

	spk::Font::Size _computeFontSize() const
	{
		return (spk::Font::Size(_height - _backgroundFrame.cornerSize().y * 2, 0));
	}

    void _onGeometryChange() override
    {
        _backgroundFrame.setGeometry({0, 0}, {geometry().size.x, _height});
        
		spk::Vector2Int anchor = _backgroundFrame.cornerSize();

        for (auto& entry : _menus)
        {
            spk::Font::Size fontSize = _computeFontSize();
            entry->menuButton->setFontSize(fontSize);
            entry->menuButton->setFontSize({fontSize.text - 4, 0}, spk::PushButton::State::Pressed);

            spk::Vector2Int buttonTextSize = entry->menuButton->computeTextSize();

            spk::Vector2UInt buttonSize = {
                buttonTextSize.x + entry->menuButton->cornerSize().x * 2 + (_height - buttonTextSize.y),
                _height - _backgroundFrame.cornerSize().y * 2
            };

            entry->menuButton->setGeometry({anchor, buttonSize});

			entry->verticalMenu->setGeometry({anchor.x, _backgroundFrame.geometry().size.y}, entry->verticalMenu->minimalSize());

            anchor.x += buttonSize.x + 5;
        }
    }

public:
    MenuBarWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
        spk::Widget(p_name, p_parent),
        _backgroundFrame(p_name + L" - BackgroundFrame", this)
    {
        _backgroundFrame.setLayer(1);
        _backgroundFrame.setCornerSize(2);
        _backgroundFrame.activate();
    }

	void setHeight(const float& p_height)
	{
		_height = p_height;
		requireGeometryUpdate();	
	}

	const float& height() const
	{
		return (_height);
	}

    spk::SafePointer<VerticalMenu> addMenu(const std::wstring& p_menuName)
    {
		std::unique_ptr<MenuEntry> newMenuEntry = std::make_unique<MenuEntry>();

        newMenuEntry->menuButton = std::make_unique<spk::PushButton>(name() + L" - " + p_menuName + L" Menu button", this);
        newMenuEntry->menuButton->setText(p_menuName);
        newMenuEntry->menuButton->setNineSlice(nullptr);
        newMenuEntry->menuButton->setCornerSize(0);
        newMenuEntry->menuButton->setLayer(10);
        newMenuEntry->menuButton->activate();

        newMenuEntry->verticalMenu = std::make_unique<VerticalMenu>(name() + L" - " + p_menuName + L" DropDown", this);
        newMenuEntry->verticalMenu->setLayer(20);

		MenuEntry* rawMenuEntryPtr = newMenuEntry.get();

        newMenuEntry->menuButtonContract = newMenuEntry->menuButton->subscribe([this, rawMenuEntryPtr]()
		{
            for(auto &entry : _menus)
            {
                entry->verticalMenu->deactivate();
            }
			if (rawMenuEntryPtr->verticalMenu->nbItem() != 0)
			{
				rawMenuEntryPtr->verticalMenu->activate();
			}
			requireGeometryUpdate();
        });

        _menus.push_back(std::move(newMenuEntry));

        requireGeometryUpdate();
        requestPaint();

        return _menus.back()->verticalMenu.get();
    }
};

template<typename TContent>
class HUDWidget : public spk::Widget
{
private:
	MenuBarWidget _menuBarWidget;
	TContent _content;

	void _onGeometryChange()
	{
		_menuBarWidget.setGeometry(geometry());
		_content.setGeometry({0, _menuBarWidget.height()}, {geometry().size.x, geometry().size.y - _menuBarWidget.height()});
	}

public:
	HUDWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_menuBarWidget(p_name + L" - MenuBarWidget", this),
		_content(p_name, this)
	{
		_content.activate();
		_menuBarWidget.activate();
	}

	spk::SafePointer<MenuBarWidget> menuBar()
	{
		return (&_menuBarWidget);
	}

	spk::SafePointer<TContent> content()
	{
		return (&_content);
	}
};

class MapEditorHUD : public HUDWidget<MapEditor>
{
private:

public:
	MapEditorHUD(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		HUDWidget<MapEditor>(p_name, p_parent)
	{
		menuBar()->setHeight(25);
		
		auto fileMenu = menuBar()->addMenu(L"File");
		fileMenu->addItem(L"Save", [&](){spk::cout << "Saving" << std::endl;});
		fileMenu->addItem(L"Load", [&](){spk::cout << "Loading" << std::endl;});

		auto editMenu = menuBar()->addMenu(L"Edit");
		editMenu->addItem(L"Undo", [&](){});
		editMenu->addItem(L"Redo", [&](){});

		auto editorMenu = menuBar()->addMenu(L"Editor");
	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {800, 800}});

	Context::instanciate();
	
	MapEditorHUD mapEditorWidget = MapEditorHUD(L"Editor window", win->widget());
	mapEditorWidget.setLayer(10);
	mapEditorWidget.setGeometry(win->geometry());
	mapEditorWidget.activate();
	
	return (app.run());
}