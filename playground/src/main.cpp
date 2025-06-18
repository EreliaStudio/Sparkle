#include <sparkle.hpp>

class TestWidget : public spk::Widget
{
private:
	spk::VerticalLayout _layout;

	spk::SpacerWidget _spacerTop;
	spk::TextLabel _label;
	spk::PushButton _button;
	spk::SpacerWidget _spacerBottom;
	
	void _onGeometryChange() override
	{
		_layout.setGeometry({0, geometry().size});
	}

public:
	TestWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_label(p_name + L"/Label", this),
		_button(p_name + L"/Button", this),
		_spacerTop(p_name + L"/SpacerTop", this),
		_spacerBottom(p_name + L"/SpacerBottom", this)
	{
		_layout.setElementPadding({10, 10});
		_layout.addWidget(&_spacerTop, spk::Layout::SizePolicy::Extend);
		_layout.addWidget(&_label, spk::Layout::SizePolicy::HorizontalExtend);
		_layout.addWidget(&_button, spk::Layout::SizePolicy::HorizontalExtend);
		_layout.addWidget(&_spacerBottom, spk::Layout::SizePolicy::Extend);

		_label.setText(L"Hello, Sparkle!");
		_label.setFontSize({16, 1});
		_label.setTextAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
		_label.setCornerSize({5, 5});
		_label.activate();	

		_button.setText(L"Click Me");
		_button.setFontSize({16, 1});
		_button.setTextAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
		_button.setCornerSize({5, 5});
		_button.activate();
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