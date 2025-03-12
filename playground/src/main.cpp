#include "playground.hpp"

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {800, 800});
	
	spk::PushButton frame = spk::PushButton(L"Main frame", win->widget());
	frame.setText(L"Click me");
	frame.setGeometry(0, 300);
	frame.activate();

	return (app.run());
}