#include "sparkle.hpp"
#include <iostream>
#include <limits>

#include <memory>
#include <utility>
#include <vector>

#define VWidget0(name, type)                  \
    auto name = std::make_unique<type>(this); \
    defer.add([&]() { vault(std::move((name))); })

#define VWidget1(name, type, param1)                    \
    auto name = std::make_unique<type>((param1).get()); \
    defer.add([&]() { vault(std::move((name))); })

#define VWidget2(name, type, param1, param2)                      \
    auto name = std::make_unique<type>((param1), (param2).get()); \
    defer.add([&]() { vault(std::move((name))); })

#define VWidget3(name, type, param1, param2, param3)                        \
    auto name = std::make_unique<type>((param1), (param2), (param3).get()); \
    defer.add([&]() { vault(std::move((name))); })

#define VWidget4(name, type, param1, param2, param3, param4)                          \
    auto name = std::make_unique<type>((param1), (param2), (param3), (param4).get()); \
    defer.add([&]() { vault(std::move((name))); })

using spk::widget::Center;
using spk::widget::ColoredBox;
using spk::widget::Column;
using spk::widget::Expanded;
using spk::widget::FractionallySizedBox;
using spk::widget::IWidget;
using spk::widget::Padding;
using spk::widget::PointerDetector;
using spk::widget::Row;
using spk::widget::SizedBox;

using spk::widget::TextLabel;

using spk::Vector2;

using std::make_unique;
using std::move;

class Defer
{
public:
    Defer() = default;
    ~Defer() { trigger(); }
    void add(const std::function<void()>& p_callback) { _callbacks.push_back(p_callback); }
    void trigger()
    {
        for (auto& c : _callbacks)
        {
            c();
        }
        _callbacks.clear();
    };

private:
    std::vector<std::function<void()>> _callbacks;
};

class WidgetVault : public spk::widget::IWidget
{
public:
    WidgetVault(const std::string& p_name, spk::widget::IWidget* p_parent = nullptr) :
        spk::widget::IWidget(p_name, p_parent)
    {
    }

protected:
    void vault(std::unique_ptr<IWidget>& item)
    {
        vault(std::move(item));
    }

    template <typename... args>
    void vault(args&&... p_stash)
    {
        auto f = [&](std::unique_ptr<IWidget>&& item)
        { vault(item); };

        (..., f(std::forward<args>(p_stash)));
    }

    void vault(std::unique_ptr<IWidget>&& item)
    {
        _vault.push_back(move(item));
    }

private:
    std::vector<std::unique_ptr<spk::widget::IWidget>> _vault;
};

class MyDemo : public WidgetVault
{
public:
    MyDemo() :
        WidgetVault("MyDemo"),
        _font("C:/Users/JQ/dev/Fonts/NotoSans/static/NotoSans-Regular.ttf")
    {
        Row* row = makeChild<Row>();
        SizedBox* sbLeft = row->makeChild<SizedBox>(Vector2{100, 10000});
        ColoredBox* coloLeft = sbLeft->makeChild<ColoredBox>(spk::Colors::orange);
        SizedBox* sbMid = row->makeChild<SizedBox>(Vector2{300, 10000});
        Padding* padMid = sbMid->makeChild<Padding>(Padding::Config::all(15));
        Column* colMid = padMid->makeChild<Column>();
        colMid->setName("Where am I.");
        const size_t max = 2;
        for (size_t i = 0; i < max; i++)
        {
            Expanded* exp = colMid->makeChild<Expanded>();
            PointerDetector* point = exp->makeChild<spk::widget::PointerDetector>();
            point->setOnPressed([&]()
                                { std::cout << i << std::endl; });
            SizedBox* sb = point->makeChild<SizedBox>(Vector2{100, 100});
            ColoredBox* cb = sb->makeChild<ColoredBox>(spk::Colors::green);
            // TextLabel* tl = cb->makeChild<TextLabel>(std::to_string(i), &_font);
            if (i < max - 1)
            {
                Padding* pad = colMid->makeChild<Padding>(Padding::Config::all(15));
            }
        }
        Expanded* expRight = row->makeChild<Expanded>();
        ColoredBox* coloRight = expRight->makeChild<ColoredBox>(spk::Colors::purple);
        Padding* padRight = coloRight->makeChild<Padding>(Padding::Config::all(15));
        ColoredBox* coloRight2 = padRight->makeChild<ColoredBox>(spk::Colors::red);
        // Column* colRight = padRight->makeChild<Column>();
        // Expanded* exp1 = colRight->makeChild<Expanded>();
        // ColoredBox* col1 = exp1->makeChild<ColoredBox>(spk::Colors::red);
        // Expanded* exp2 = colRight->makeChild<Expanded>();
        // ColoredBox* col2 = exp2->makeChild<ColoredBox>(spk::Colors::yellow);
    }

private:
    spk::Font _font;
};

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    MyDemo demo;
    demo.activateAll();

    return (app.run());
}
