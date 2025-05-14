#include <sparkle.hpp>

int main() {
    spk::GraphicalApplication app;

    auto win = app.createWindow(L"Layout Playground", {{0, 0}, {1024, 768}});

    spk::MultilineTextLabel screen(L"LayoutTestScreen", win->widget());
	screen.setText(L"Ceci est un test\nMais sur plusieurs lignes");
    screen.setGeometry(win->geometry());
    screen.activate();

    return app.run();
}
