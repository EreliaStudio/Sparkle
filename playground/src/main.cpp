#include "playground.hpp"


class MenuBarWidget : public spk::Widget
{
private:
	spk::Frame _backgroundFrame;
	std::vector<std::unique_ptr<spk::PushButton>> _menuButtons;

	void _onGeometryChange() override
	{
		_backgroundFrame.setGeometry({0, 0}, geometry().size);
		spk::Vector2Int anchor = _backgroundFrame.cornerSize();
		for (auto& button : _menuButtons)
		{
			size_t glyphSize = geometry().size.y - button->cornerSize().y * 2 - _backgroundFrame.cornerSize().y * 2;

			button->setFontSize({glyphSize, 0});
			button->setFontSize({glyphSize - 4, 0}, spk::PushButton::State::Pressed);
			spk::Vector2Int buttonTextSize = button->computeTextSize();

			float spaceLeft = (geometry().size.y - buttonTextSize.y) / 2.0f;

			spk::Vector2UInt buttonSize = {
				buttonTextSize.x + button->cornerSize().x * 2 + spaceLeft * 2,
				geometry().size.y - _backgroundFrame.cornerSize().y * 2
			};

			button->setGeometry({anchor, buttonSize});
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

	void addMenu(const std::wstring& p_menuName)
	{
		std::unique_ptr<spk::PushButton> button = std::make_unique<spk::PushButton>(name() + L" - " + p_menuName + L" Menu button", this);

		button->setText(p_menuName);
		button->setNineSlice(nullptr);
		button->setCornerSize(0);
		button->setLayer(10);
		button->activate();

		_menuButtons.emplace_back(std::move(button));

		requireGeometryUpdate();
		requestPaint();
	}
};

template<typename TContent>
class HUDWidget : public spk::Widget
{
private:
	float _menuBarHeight = 25;
	MenuBarWidget _menuBarWidget;
	TContent _content;

	void _onGeometryChange()
	{
		_menuBarWidget.setGeometry({0, 0}, {geometry().size.x, _menuBarHeight});
		_content.setGeometry({0, _menuBarHeight}, {geometry().size.x, geometry().size.y - _menuBarHeight});
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

	void setMenuBarHeight(const float& p_height)
	{
		_menuBarHeight = p_height;
		requireGeometryUpdate();	
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
		setMenuBarHeight(25);
		
		menuBar()->addMenu(L"File");
		menuBar()->addMenu(L"Edit");
		menuBar()->addMenu(L"Editor");
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