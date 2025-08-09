# Simple Calculator

Build a tiny calculator with Sparkle widgets. We start by manually placing
digit buttons, then switch to a layout, and finally add a display with a few
operations.

## 1. Create the application window

Begin with a minimal `main` function that creates a window and runs the event
loop.

```cpp
int main()
{
    spk::GraphicalApplication app;
    auto window = app.createWindow(L"Calculator", {{0,0},{300,400}});

    return app.run();
}
```

## 2. Manually place digit buttons

Create a widget that owns ten `spk::PushButton` objects and positions them
manually inside `onGeometryChange`.

```cpp
#include <sparkle.hpp>

class ManualDigitsWidget : public spk::Widget
{
    std::array<std::unique_ptr<spk::PushButton>,10> _digits;

    void _onGeometryChange() override
    {
        spk::Vec2i cell = {geometry().size.x/3, geometry().size.y/4};
        for (int i = 0; i < 10; ++i)
        {
            int col = i % 3;
            int row = i / 3;
            spk::Vec2i pos = {col*cell.x, row*cell.y};
            _digits[i]->setGeometry({pos, cell});
        }
    }

public:
    ManualDigitsWidget(const std::wstring& name, spk::SafePointer<spk::Widget> parent)
        : spk::Widget(name, parent)
    {
        for (int i = 0; i < 10; ++i)
        {
            _digits[i] = std::make_unique<spk::PushButton>(name + L"/" + std::to_wstring(i), this);
            _digits[i]->setText(std::to_wstring(i));
            _digits[i]->activate();
        }
    }
};
```

Instantiate it:

```cpp
int main()
{
    spk::GraphicalApplication app;
    auto window = app.createWindow(L"Calculator", {{0,0},{300,400}});

    ManualDigitsWidget digits(L"Digits", window->widget());
    digits.setGeometry(window->geometry());
    digits.activate();

    return app.run();
}
```

At this stage the buttons appear but every position is hard coded.

## 3. Replace manual positioning with a layout

Layouts automatically handle geometry. Swap the manual placement for a
`spk::GridLayout<3,4>` that arranges the digits.

```cpp
class DigitLayoutWidget : public spk::Widget
{
    spk::GridLayout<3,4> _layout;
    std::array<std::unique_ptr<spk::PushButton>,10> _digits;

    void _onGeometryChange() override
    {
        _layout.setGeometry({0, geometry().size});
    }

public:
    DigitLayoutWidget(const std::wstring& name, spk::SafePointer<spk::Widget> parent)
        : spk::Widget(name, parent)
    {
        for (int i = 0; i < 10; ++i)
        {
            _digits[i] = std::make_unique<spk::PushButton>(name + L"/" + std::to_wstring(i), this);
            _digits[i]->setText(std::to_wstring(i));
            _digits[i]->activate();
        }

        for (int i = 1; i <= 9; ++i)
        {
            int col = (i-1)%3;
            int row = (i-1)/3;
            _layout.setWidget(col, row, _digits[i].get(), spk::Layout::SizePolicy::Extend);
        }
        _layout.setWidget(1,3,_digits[0].get(), spk::Layout::SizePolicy::Extend);
    }
};
```

Use it in `main` exactly like before. The layout now arranges the buttons and
resizes them when the window changes.

## 4. Add the display and operations

Extend the widget with a `spk::TextLabel` for the current value and three more
buttons. Hook up callbacks to make the calculator add numbers.

```cpp
class CalculatorWidget : public spk::Widget
{
    spk::GridLayout<4,5> _layout;
    spk::TextLabel _display;
    std::array<std::unique_ptr<spk::PushButton>,10> _digits;
    spk::PushButton _plus;
    spk::PushButton _equals;
    spk::PushButton _clear;

    int _accumulator = 0;
    bool _waitingSecond = false;

    void _onGeometryChange() override
    {
        _layout.setGeometry({0, geometry().size});
    }

public:
    CalculatorWidget(const std::wstring& name, spk::SafePointer<spk::Widget> parent)
        : spk::Widget(name, parent),
          _display(name + L"/Display", this),
          _plus(name + L"/Plus", this),
          _equals(name + L"/Equals", this),
          _clear(name + L"/Clear", this)
    {
        _layout.setElementPadding({5,5});

        _display.setFontSize({20,1});
        _display.setTextAlignment(spk::HorizontalAlignment::Right,
                                  spk::VerticalAlignment::Centered);
        _display.activate();
        _layout.setWidget(0,0,&_display, spk::Layout::SizePolicy::HorizontalExtend);

        for (int i = 0; i < 10; ++i)
        {
            _digits[i] = std::make_unique<spk::PushButton>(name + L"/" + std::to_wstring(i), this);
            _digits[i]->setText(std::to_wstring(i));
            _digits[i]->activate();
            _digits[i]->subscribe([this, i]() {
                _display.setText(_display.text() + std::to_wstring(i));
            });
        }

        for (int i = 1; i <= 9; ++i)
        {
            int col = (i-1)%3;
            int row = 1 + (i-1)/3;
            _layout.setWidget(col, row, _digits[i].get(), spk::Layout::SizePolicy::Extend);
        }
        _layout.setWidget(1,4,_digits[0].get(), spk::Layout::SizePolicy::Extend);

        _plus.setText(L"+");
        _equals.setText(L"=");
        _clear.setText(L"C");
        _plus.activate();
        _equals.activate();
        _clear.activate();

        _layout.setWidget(3,1,&_plus);
        _layout.setWidget(3,2,&_equals);
        _layout.setWidget(3,3,&_clear);

        _plus.subscribe([this]() {
            _accumulator = std::stoi(_display.text());
            _display.setText(L"");
            _waitingSecond = true;
        });

        _equals.subscribe([this]() {
            if (_waitingSecond)
            {
                _accumulator += std::stoi(_display.text());
                _display.setText(std::to_wstring(_accumulator));
                _waitingSecond = false;
            }
        });

        _clear.subscribe([this]() {
            _display.setText(L"");
            _accumulator = 0;
            _waitingSecond = false;
        });
    }
};
```

Instantiate the final widget:

```cpp
int main()
{
    spk::GraphicalApplication app;
    auto window = app.createWindow(L"Calculator", {{0,0},{300,400}});

    CalculatorWidget calc(L"Calc", window->widget());
    calc.setGeometry(window->geometry());
    calc.activate();

    return app.run();
}
```

Running the program now shows a functional calculator. The tutorial illustrated
how to manually place widgets, adopt layouts, and combine widgets into an
interactive application.

