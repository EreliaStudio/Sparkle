#include <sparkle.hpp>

class CustomWidget : public spk::Widget
{
private:
	spk::VerticalLayout _layout;

	spk::Vector2UInt _clickPosition;

	spk::Frame _frame;

	void _onGeometryChange() override
	{
		_layout.setGeometry({geometry().anchor + _clickPosition, geometry().size});
	}

	void _onMouseEvent(spk::MouseEvent &p_event) override
	{
		if (p_event.type == spk::MouseEvent::Type::Press &&
			p_event.button == spk::Mouse::Button::Left)
		{
			_clickPosition = p_event.mouse->position - (geometry().size / 2);
			requireGeometryUpdate();
		}
	}

public:
	CustomWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_frame(p_name + L"/Frame", this)
	{
		_layout.setElementPadding({10, 10});	

		_layout.addWidget(&_frame, spk::Layout::SizePolicy::Extend);

		_frame.setCornerSize(16);
		_frame.activate();
	}

	spk::Vector2UInt minimalSize() const override
	{
		return (spk::Vector2UInt{2000, 2000});
	}
};

class MainWidget : public spk::Widget
{
private:
	spk::ScrollArea<CustomWidget> _scrollArea;

	void _onGeometryChange() override
	{
		_scrollArea.setGeometry({geometry().anchor + 50, geometry().size - 100});
	}

public:
	MainWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_scrollArea(L"ScrollArea", this)
	{
		_scrollArea.activate();
	}
};

int main() {
    spk::GraphicalApplication app;

    auto win = app.createWindow(L"Check scroll area - Playground", {{0, 0}, {1024, 768}});

	MainWidget test(L"MainWidget", win->widget());
	test.setGeometry(0, win->geometry().size);
	test.activate();

    return app.run();
}
