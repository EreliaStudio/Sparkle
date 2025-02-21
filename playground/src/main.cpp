#include "playground.hpp"

class MapEditorHUD : public spk::HUDWidget<MapEditor>
{
private:

public:
	MapEditorHUD(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::HUDWidget<MapEditor>(p_name, p_parent)
	{
		menuBar()->setHeight(25);
		
		spk::SafePointer<spk::MenuBar::Menu> fileMenu = menuBar()->addMenu(L"File");

		spk::SafePointer<spk::MenuBar::Menu> editMenu = menuBar()->addMenu(L"Edit");

		spk::SafePointer<spk::MenuBar::Menu> editorMenu = menuBar()->addMenu(L"Editor");
		editorMenu->addItem(L"Save", [&](){EventCenter::instance()->notifyEvent(Event::SaveMap);});
		editorMenu->addItem(L"Load", [&](){EventCenter::instance()->notifyEvent(Event::LoadMap);});
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