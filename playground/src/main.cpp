#include <sparkle.hpp>

int main()
{
	spk::GraphicalApplication app;

	spk::SafePointer<spk::Window> win = app.createWindow(L"Erelia", {{0, 0}, {800, 600}});

	spk::PushButton testWidget(L"Test Widget", win->widget());
	testWidget.setCornerSize(32);
	testWidget.setGeometry(win->geometry().shrink(30));
	testWidget.setLayer(1.0f);
	testWidget.activate();

	spk::PushButton testWidget2(L"Test Widget 2", win->widget());
	testWidget2.setCornerSize(32);
	testWidget2.setGeometry({{100, 100}, {200, 200}});
	testWidget2.setLayer(2.0f);
	testWidget2.activate();

	return (app.run());
}