#include <sparkle.hpp>

class TestWidget : public spk::Widget
{
private:	
	spk::ColorRenderer _colorRenderer;

	void _onGeometryChange() override
	{
		_colorRenderer.clear();
		_colorRenderer.prepareSquare({{0, 0}, geometry().size}, layer());
		_colorRenderer.validate();
	}

	void _onPaintEvent(spk::PaintEvent& p_event)
	{
		_colorRenderer.render();
	}

public:
	TestWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_colorRenderer.setColor(spk::Color::red);
	}
};

int main()
{
	spk::GraphicalApplication app;

	spk::SafePointer<spk::Window> win = app.createWindow(L"Erelia", {{0, 0}, {800, 600}});

	TestWidget testWidget(L"Test Widget", win->widget());
	testWidget.setGeometry(win->geometry());
	testWidget.activate();

	return (app.run());
}