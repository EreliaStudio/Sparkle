#include <sparkle.hpp>

int main() {
    spk::GraphicalApplication app;

    auto win = app.createWindow(L"Layout Playground", {{0, 0}, {1024, 768}});

    spk::MessageBox box(L"DemoBox", win->widget());

	box.textArea().setTextAlignment(spk::HorizontalAlignment::Right, spk::VerticalAlignment::Top);

	box.setText(L"Save changes before closing this document?");

	box.addButton(L"YesBtn",    L"Yes");
	box.addButton(L"NoBtn",     L"No");
	box.addButton(L"CancelBtn", L"Cancel");

	box.setMinimalWidth(300);

	spk::Vector2UInt boxSize  = {30, 180};
	spk::Vector2Int  boxPos   = (win->geometry().size - boxSize) / 2;
	box.setGeometry({boxPos, boxSize});

	box.activate();

    return app.run();
}
