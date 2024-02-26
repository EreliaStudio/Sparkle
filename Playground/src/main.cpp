#include "sparkle.hpp"
#include <iostream>
#include <limits>

#include <memory>
#include <utility>
#include <vector>

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

class MyDemo : public IWidget
{
public:
    MyDemo(spk::Font* p_font) :
        IWidget("MyDemo"),
        _font(p_font)
    {
        Row* row = makeChild<Row>();
        SizedBox* sbLeft = row->makeChild<SizedBox>(Vector2{50, 10000});
        SizedBox* sbMid = row->makeChild<SizedBox>(Vector2{300, 10000});
        ColoredBox* colorMid = sbMid->makeChild<ColoredBox>(spk::Colors::red);
        Column* colMid = colorMid->makeChild<Column>();
        colMid->setName("Where am I.");
        const size_t max = 5;
        for (size_t i = 0; i < max; i++)
        {
            Padding* pad;
            if (i == 0)
            {
                pad = colMid->makeChild<Padding>(Padding::Config::all(15));
            }
            else
            {
                pad = colMid->makeChild<Padding>(Padding::Config(15, 15, 1, 15));
            }
            PointerDetector* point = pad->makeChild<spk::widget::PointerDetector>();
            point->setOnPressed([i]()
                                { std::cout << i << std::endl; });
            SizedBox* sb = point->makeChild<SizedBox>(Vector2{10000, 60});
            ColoredBox* cb = sb->makeChild<ColoredBox>(spk::Colors::black);
            cb->setDepth(2);
            TextLabel* tl = cb->makeChild<TextLabel>(std::to_string(i), _font);
            tl->setDepth(1);
        }
        Expanded* expRight = row->makeChild<Expanded>();
        {
            Column* col = expRight->makeChild<Column>();
            // topright
            {
                Expanded* exp = col->makeChild<Expanded>();
                ColoredBox* color = exp->makeChild<ColoredBox>(spk::Colors::green);
                color->setDepth(10);
                Padding* pad = color->makeChild<Padding>(Padding::Config::all(15));
                {
                    Column* col = pad->makeChild<Column>();
                    {
                        Expanded* exp = col->makeChild<Expanded>();
                        PointerDetector* point = exp->makeChild<spk::widget::PointerDetector>();
                        point->setOnPressed([]()
                                            { std::cout << "toto" << std::endl; });
                        ColoredBox* cb = point->makeChild<ColoredBox>(spk::Colors::black);
                        cb->setDepth(2);
                        TextLabel* tl = cb->makeChild<TextLabel>("toto", _font);
                        tl->setDepth(1);
                    }
                    col->makeChild<SizedBox>(Vector2(10000, 15));
                    {
                        Expanded* exp = col->makeChild<Expanded>();
                        PointerDetector* point = exp->makeChild<spk::widget::PointerDetector>();
                        point->setOnPressed([]()
                                            { std::cout << "tata" << std::endl; });
                        ColoredBox* cb = point->makeChild<ColoredBox>(spk::Colors::black);
                        cb->setDepth(2);
                        TextLabel* tl = cb->makeChild<TextLabel>("tata", _font);
                        tl->setDepth(1);
                    }
                }
            }
            // bottomright
            {
                Expanded* exp = col->makeChild<Expanded>();
                Center* center = exp->makeChild<Center>();
                spk::widget::LayoutBuilder* builder = center->makeChild<spk::widget::LayoutBuilder>(
                    [](const spk::widget::BoxConstraints& constraints, IWidget* child)
                    {
                        Vector2 min = constraints.min / 2;
                        Vector2 max = constraints.max / 2;
                        spk::widget::BoxConstraints newConstraints{min, max};
                        if (child != nullptr)
                        {
                            Vector2 childSize = child->layout(newConstraints);
                            child->setGeometry({0, 0}, childSize);
                        }
                        return max;
                    });
                ColoredBox* color = builder->makeChild<ColoredBox>(spk::Colors::cyan);
            }
        }
        // Column* colRight = padRight->makeChild<Column>();
        // Expanded* exp1 = colRight->makeChild<Expanded>();
        // ColoredBox* col1 = exp1->makeChild<ColoredBox>(spk::Colors::red);
        // Expanded* exp2 = colRight->makeChild<Expanded>();
        // ColoredBox* col2 = exp2->makeChild<ColoredBox>(spk::Colors::yellow);
    }

private:
    spk::Font* _font;
};

int main(int argc, char* argv[])
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(816, 640), spk::Application::Mode::Monothread);
    spk::Font font{argv[1]};
    MyDemo demo{&font};
    demo.activateAll();

    return (app.run());
}
