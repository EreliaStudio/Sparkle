#include "sparkle.hpp"
#include <iostream>
#include <limits>

#include <utility>
#include <vector>

#define VWidget1(name, type, param1)              \
    auto name = std::make_unique<type>((param1)); \
    defer.add([&]() { vault(std::move((name))); })

#define VWidget2(name, type, param1, param2)                \
    auto name = std::make_unique<type>((param1), (param2)); \
    defer.add([&]() { vault(std::move((name))); })

#define VWidget3(name, type, param1, param2, param3)                  \
    auto name = std::make_unique<type>((param1), (param2), (param3)); \
    defer.add([&]() { vault(std::move((name))); })

#define VWidget4(name, type, param1, param2, param3, param4)                    \
    auto name = std::make_unique<type>((param1), (param2), (param3), (param4)); \
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
        auto row = make_unique<Row>(this);

        // Left empty padding
        {
            auto leftPad = make_unique<SizedBox>(Vector2{50.0f, 5000.0f}, row.get());
            vault(move(leftPad));
        }

        // Middle column
        {
            auto padding = make_unique<Padding>(Padding::Config{15.0f, 15.0f, 15.0f, 15.0f}, row.get());
            auto col = make_unique<Column>(padding.get());

            std::vector<std::string> labels{"LabelA", "LabelB", "LabelC", "LabelD", "LabelE"};
            for (size_t i = 0; i < labels.size(); i++)
            {
                auto callback = [label = labels[i]]()
                { std::cout << label << std::endl; };
                vault(make_unique<TextLabel>(labels[i], &_font, col.get()));

                // Only build separators until the last one.
                if (i < (labels.size() - 1))
                {
                    vault(make_unique<SizedBox>(Vector2{10.0f, 10.0f}, col.get()));
                }
            }
            vault(move(col));
            vault(move(padding));
        }

        // Right column
        {
            auto expanded = make_unique<Expanded>(row.get());
            auto col = make_unique<Column>(expanded.get());
            // Top section
            {
                auto expanded = make_unique<Expanded>(col.get());
                auto padding = make_unique<Padding>(Padding::Config{15, 15, 15, 15}, expanded.get());
                auto innerCol = make_unique<Column>(padding.get());
                auto top = make_unique<Expanded>(innerCol.get());
                auto topButton = make_unique<TextLabel>(
                    "TopButton",
                    &_font,
                    top.get());
                auto separator = make_unique<SizedBox>(Vector2{10, 15}, innerCol.get());
                auto bot = make_unique<Expanded>(innerCol.get());
                auto botButton = make_unique<TextLabel>(
                    "BotButton", &_font, top.get());
                vault(move(expanded), move(padding), move(innerCol), move(top), move(topButton), move(separator), move(bot), move(botButton));
            }
            // Bottom section
            {
                auto expanded = make_unique<Expanded>(col.get());
                auto center = make_unique<Center>(expanded.get());
                auto frac = make_unique<FractionallySizedBox>(0.5f, 0.5f, center.get());
                auto button = make_unique<TextLabel>(
                    "Second button",
                    &_font, frac.get());
                vault(move(expanded), move(center), move(frac), move(button));
            }
            vault(move(col), move(expanded));
        }
        vault(move(row));
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
        // VWidget1(center, Center, this);
        // VWidget2(sb, SizedBox, Vector2(400, 400), center.get());
        VWidget2(redBox, spk::widget::ColoredBox, spk::Color(255, 0, 0), this);
        // VWidget3(_label, spk::widget::TextLabel, "Bobo", &font, redBox.get());

        //_label->label().setFont(&font); // Assuming myFont is a preloaded Font instance
        //_label->label().setText("Bobo");
        //_label->label().setTextSize(50);
        //_label->label().setTextColor(spk::Color{255, 255, 255});
        //_label->label().setAnchor({100, 100});
        //_label->label().setVerticalAlignment(spk::VerticalAlignment::Centered);
        //_label->label().setHorizontalAlignment(spk::HorizontalAlignment::Left);
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
