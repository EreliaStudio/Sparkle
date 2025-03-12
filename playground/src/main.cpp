#include "playground.hpp"

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {800, 800});
	
	spk::DebugOverlay<5, 15> frame = spk::DebugOverlay<5, 15>(L"Main frame", win->widget());

	for (size_t i = 0; i < frame.Column; i++)
	{
		for (size_t j = 0; j < frame.Row; j++)
		{
			frame.setText(i, j, L"Text label [" + std::to_wstring(i) + L"][" + std::to_wstring(j) + L"]");
		}
	}
	frame.setText(0, 0, L"Click me");
	frame.setGeometry(0, win->geometry().size);
	frame.activate();

	return (app.run());
}