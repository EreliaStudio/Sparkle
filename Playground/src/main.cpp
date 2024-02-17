#include "sparkle.hpp"
#include <iostream>
#include <limits>

#include <utility>
#include <vector>

using spk::widget::Center;
using spk::widget::Column;
using spk::widget::Expanded;
using spk::widget::FractionallySizedBox;
using spk::widget::Padding;
using spk::widget::Row;
using spk::widget::SizedBox;

using spk::widget::TextButton;

using spk::Vector2;

using std::make_unique;
using std::move;

class MyDemo : public spk::widget::SingleChildWidget
{
public:
    MyDemo() :
        SingleChildWidget("MyDemo"),
        _font("C:/Users/JQ/dev/Fonts/NotoSans/static/NotoSans-Regular.ttf")
    {
        auto row = make_unique<Row>(this);

        // Left empty padding
        {
            auto leftPad = make_unique<SizedBox>(Vector2{50.0f, std::numeric_limits<float>::max()}, row.get());
            _vault.push_back(move(leftPad));
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
                vault(make_unique<TextButton>(labels[i], callback, &_font, col.get()));

                // Only build separators until the last one.
                if (i < (labels.size() - 1))
                {
                    vault(make_unique<SizedBox>(Vector2{0.0f, 10.0f}, col.get()));
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
                auto topButton = make_unique<TextButton>(
                    "TopButton", []()
                    { std::cout << "TopButton" << std::endl; },
                    &_font,
                    top.get());
                auto separator = make_unique<SizedBox>(Vector2{0, 15}, innerCol.get());
                auto bot = make_unique<Expanded>(innerCol.get());
                auto botButton = make_unique<TextButton>(
                    "BotButton", []()
                    { std::cout << "BotButton" << std::endl; },
                    &_font, top.get());
                vault(move(expanded), move(padding), move(innerCol), move(top), move(topButton), move(separator), move(bot), move(botButton));
            }
            // Bottom section
            {
                auto expanded = make_unique<Expanded>(col.get());
                auto center = make_unique<Center>(expanded.get());
                auto frac = make_unique<FractionallySizedBox>(0.5f, 0.5f, center.get());
                auto button = make_unique<TextButton>(
                    "Second button", []()
                    { std::cout << "Second button" << std::endl; },
                    &_font, frac.get());
                vault(move(expanded), move(center), move(frac), move(button));
            }
            vault(move(col), move(expanded));
        }
        vault(move(row));
    }

private:
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

    std::vector<std::unique_ptr<IWidget>> _vault;
    spk::Font _font;
};

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    MyDemo demo;
    demo.activateAll();

    return (app.run());
}
