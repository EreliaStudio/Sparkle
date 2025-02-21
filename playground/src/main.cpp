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
		fileMenu->addItem(L"Save", [&](){spk::cout << "Saving" << std::endl;});
		fileMenu->addItem(L"Load", [&](){spk::cout << "Loading" << std::endl;});
		fileMenu->addBreak()->setHeight(3);
		fileMenu->addItem(L"Undo", [&](){});
		fileMenu->addItem(L"Redo", [&](){});

		spk::SafePointer<spk::MenuBar::Menu> editMenu = menuBar()->addMenu(L"Edit");
		editMenu->addItem(L"Undo", [&](){});
		editMenu->addItem(L"Redo", [&](){});

		spk::SafePointer<spk::MenuBar::Menu> editorMenu = menuBar()->addMenu(L"Editor");
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