#include <sparkle.hpp>

class GridDemo : public spk::Widget {
private:
    spk::GridLayout<3, 3>                                   _layout;      // 3×3 grid
    std::vector<std::unique_ptr<spk::PushButton>>            _buttons;

    void _onGeometryChange() override { _layout.setGeometry(geometry()); }

public:
    GridDemo(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent)
        : spk::Widget(p_name, p_parent) {
        _layout.setElementPadding({5, 5});

        for (std::size_t r = 0; r < 3; ++r) {
            for (std::size_t c = 0; c < 3; ++c) {
                std::wstring btnName = p_name + L"/Btn_" + std::to_wstring(r) + L"_" + std::to_wstring(c);
                auto btn = std::make_unique<spk::PushButton>(btnName, this);
                btn->setText(L"(" + std::to_wstring(r) + L"," + std::to_wstring(c) + L")");
                btn->activate();

                _layout.setWidget(c, r, btn.get(), spk::Layout::SizePolicy::Extend);
                _buttons.push_back(std::move(btn));
            }
        }
    }
};

class HorizontalDemo : public spk::Widget {
private:
    spk::HorizontalLayout  _layout;
    std::vector<std::unique_ptr<spk::PushButton>> _buttons;

    void _onGeometryChange() override { _layout.setGeometry(geometry()); }

public:
    HorizontalDemo(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent)
        : spk::Widget(p_name, p_parent) {
        _layout.setElementPadding({5, 0});

        for (int i = 0; i < 5; ++i) {
            std::wstring btnName = p_name + L"/HBtn_" + std::to_wstring(i);
            auto btn = std::make_unique<spk::PushButton>(btnName, this);
            btn->setText(L"H" + std::to_wstring(i));
            btn->activate();

            _layout.addWidget(btn.get(), spk::Layout::SizePolicy::Extend);
            _buttons.push_back(std::move(btn));
        }
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class FormDemo : public spk::Widget {
private:
    spk::FormLayout _layout;
    std::vector<std::unique_ptr<spk::TextLabel>> _labels;
    std::vector<std::unique_ptr<spk::TextEdit>> _fields;

    void _onGeometryChange() override { _layout.setGeometry(geometry()); }

public:
    FormDemo(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent)
        : spk::Widget(p_name, p_parent) {
        _layout.setElementPadding({5, 5});

        std::array<std::wstring, 3> captions = {L"Player Name", L"Difficulty", L"Seed"};
        for (std::size_t i = 0; i < captions.size(); ++i) {
            std::wstring lblName   = p_name + L"/Lbl_"   + std::to_wstring(i);
            std::wstring fieldName = p_name + L"/Field_" + std::to_wstring(i);

            auto lbl   = std::make_unique<spk::TextLabel>(lblName, this);
            lbl->setText(captions[i]);
            lbl->activate();

            auto field = std::make_unique<spk::TextEdit>(fieldName, this);
            field->setText(L"");
            field->activate();

            _layout.addRow(lbl.get(), field.get(), spk::Layout::SizePolicy::Minimum, spk::Layout::SizePolicy::Extend);

            _labels.push_back(std::move(lbl));
            _fields.push_back(std::move(field));
        }
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Root screen that stacks the three demos vertically                                         ////
///////////////////////////////////////////////////////////////////////////////////////////////////

class LayoutTestScreen : public spk::Screen {
private:
    GridDemo           _gridDemo;
    HorizontalDemo     _horizontalDemo;
    FormDemo           _formDemo;

    void _onGeometryChange() override
	{
		_gridDemo.setGeometry(geometry());
		_horizontalDemo.setGeometry(geometry());
		_formDemo.setGeometry(geometry());
	}

	void _onKeyboardEvent(spk::KeyboardEvent& p_event) override
	{
		if (p_event.type == spk::KeyboardEvent::Type::Press)
		{
			if (p_event.key == spk::Keyboard::A)
			{
				_gridDemo.activate();
				_horizontalDemo.deactivate();
				_formDemo.deactivate();
			}
			if (p_event.key == spk::Keyboard::Z)
			{
				_gridDemo.deactivate();
				_horizontalDemo.activate();
				_formDemo.deactivate();
			}
			if (p_event.key == spk::Keyboard::E)
			{
				_gridDemo.deactivate();
				_horizontalDemo.deactivate();
				_formDemo.activate();
			}
		}
	}

public:
    LayoutTestScreen(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent)
        : spk::Screen(p_name, p_parent),
          _gridDemo(p_name + L"/GridDemo", this),
          _horizontalDemo(p_name + L"/HorizontalDemo", this),
          _formDemo(p_name + L"/FormDemo", this)
	{
        //_gridDemo.activate();
        _horizontalDemo.activate();
        //_formDemo.activate();
    }
};

int main() {
    spk::GraphicalApplication app;

    auto win = app.createWindow(L"Layout Playground", {{0, 0}, {1024, 768}});

    LayoutTestScreen screen(L"LayoutTestScreen", win->widget());
    screen.setGeometry(win->geometry());
    screen.activate();

    return app.run();
}
