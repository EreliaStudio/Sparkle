#include <sparkle.hpp>

int main() {
    spk::GraphicalApplication app;

    auto win = app.createWindow(L"Layout Playground", {{0, 0}, {1024, 768}});

    spk::MultilineTextLabel screen(L"LayoutTestScreen", win->widget());
	screen.setText(L"Ceci est un        test un        peu trop long\nCeci est un deuxieme test\nCeci est un troisieme test");
    screen.setGeometry({win->geometry().size / 4, win->geometry().size / 2});
    screen.activate();

    return app.run();
}
