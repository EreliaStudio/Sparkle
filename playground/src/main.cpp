#include <sparkle.hpp>

class TestWidget : public spk::Widget
{
private:	
	spk::ColorRenderer _colorRenderer;

	void _onGeometryChange() override
	{
		_colorRenderer.clear();
		_colorRenderer.prepareSquare({{0, 0}, geometry().size}, 0);
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

	void setColor(const spk::Color& p_color)
	{
		_colorRenderer.setColor(p_color);
	}
};

int main()
{
	spk::GraphicalApplication app;

	spk::SafePointer<spk::Window> win = app.createWindow(L"Erelia", {{0, 0}, {800, 600}});

	TestWidget testWidget(L"Test Widget", win->widget());
	testWidget.setGeometry(win->geometry());
	testWidget.setColor(spk::Color::blue);
	testWidget.setLayer(1.0f);
	testWidget.activate();

	TestWidget testWidget2(L"Test Widget 2", win->widget());
	testWidget2.setGeometry({{100, 100}, {200, 200}});
	testWidget2.setColor(spk::Color::green);
	testWidget2.setLayer(0.0f);
	testWidget2.activate();

	return (app.run());
}