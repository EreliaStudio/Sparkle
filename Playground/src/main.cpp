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
using spk::widget::Column;
using spk::widget::Expanded;
using spk::widget::FractionallySizedBox;
using spk::widget::IWidget;
using spk::widget::Padding;
using spk::widget::Row;
using spk::widget::SizedBox;

using spk::widget::TextLabel;

using spk::Vector2;

using std::make_unique;
using std::move;

spk::Font font("Playground/upheavtt.ttf");

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
    }

private:
    spk::Font _font;
};

class EasyDemo : public WidgetVault
{
public:
    EasyDemo() :
        WidgetVault("EasyDemo", nullptr)
    {
        Defer defer;
        VWidget0(center, Center);
        VWidget1(col, Column, center);
        VWidget2(sb, SizedBox, Vector2(400, 400), col);
        VWidget2(redBox, spk::widget::ColoredBox, spk::Color(255, 0, 0), sb);
        redBox->setName("redBox");
        VWidget3(_label, spk::widget::TextLabel, "Bobo", &font, redBox);

        VWidget1(exp, Expanded, col);
        VWidget2(greenBox, spk::widget::ColoredBox, spk::Color(0, 255, 0), exp);
        greenBox->setName("greenBox");
        VWidget3(_label2, spk::widget::TextLabel, "Popo", &font, greenBox);
        _label2->setName("Popo");

        VWidget1(exp2, Expanded, col);
        spk::widget::Row::Config rowConfig;
        rowConfig.crossAxisAlignment = spk::widget::CrossAxisAlignment::center;
        VWidget2(row, Row, rowConfig, exp2);
        for (int i = 0; i < 16; i++)
        {
            int colorIndex = i % (spk::Colors::values().size());
            auto e = std::make_unique<spk::widget::Expanded>(row.get());
            auto sz = std::make_unique<spk::widget::SizedBox>(spk::Vector2{10 * (i + 1), 10 * (i + 1)}, e.get());
            auto box = std::make_unique<spk::widget::ColoredBox>(spk::Colors::values()[colorIndex], sz.get());
            vault(std::move(box), std::move(e), std::move(sz));
        }

        redBox->setDepth(100);
        greenBox->setDepth(100);

        _label2->setDepth(1);
        _label->setDepth(1);

        _label->label().setFont(&font); // Assuming myFont is a preloaded Font instance
        _label->label().setText("Bobo");
        _label->label().setTextSize(50);
        _label->label().setTextColor(spk::Color{255, 255, 255});
        defer.trigger();
        this->activateAll();
    }
};

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    // MyDemo demo;
    // demo.activateAll();

    EasyDemo demo;
    demo.activateAll();

    return (app.run());
}
