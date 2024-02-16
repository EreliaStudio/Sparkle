#include "sparkle.hpp"
#include <iostream>
#include <limits>

#include "spk_button.hpp"
#include "spk_frame.hpp"
#include "spk_text_label.hpp"

class MainWidget : public spk::IWidget
{
private:
    spk::SpriteSheet _redBackgroundTexture;
    spk::SpriteSheet _darkGreenBackgroundTexture;
    spk::SpriteSheet _blueBackgroundTexture;

    spk::Frame _listFrame;
    spk::Frame _twoButtonFrame;

    spk::Button _blueButton;

    void _onGeometryChange()
    {
        _listFrame.setGeometry(spk::Vector2Int(50, 0), spk::Vector2Int(300, size().y));

        spk::Vector2Int twoButtonFrameAnchor = spk::Vector2Int(_listFrame.anchor().x + _listFrame.size().x, 0);
        _twoButtonFrame.setGeometry(twoButtonFrameAnchor, spk::Vector2Int(size().x - twoButtonFrameAnchor.x, size().y / 2));

        spk::Vector2Int buttonHiddedAnchor = twoButtonFrameAnchor + spk::Vector2Int(0, _twoButtonFrame.size().y);
        spk::Vector2Int leftover = size() - buttonHiddedAnchor;
        _blueButton.setGeometry(buttonHiddedAnchor + leftover / 4, leftover / 2);
    }

    void _onRender()
    {

    }

    void _onUpdate()
    {
        
    }

public:
    MainWidget(const std::string& p_name) :
        spk::IWidget(p_name),
        _listFrame("ListPanel"),
        _redBackgroundTexture("9SlicedRed.png", spk::Vector2Int(3, 3)),
        _twoButtonFrame("TwoButtonPanel"),
        _darkGreenBackgroundTexture("9SlicedDarkGreen.png", spk::Vector2Int(3, 3)),
        _blueButton("BlueButton", [&](){std::cout << "Coucou !" << std::endl;}),
        _blueBackgroundTexture("9SlicedLightBlue.png", spk::Vector2Int(3, 3))
    {
        _listFrame.box().setSpriteSheet(&_redBackgroundTexture);
        _listFrame.box().setCornerSize(32);
        _listFrame.activate();

        _twoButtonFrame.box().setSpriteSheet(&_darkGreenBackgroundTexture);
        _twoButtonFrame.box().setCornerSize(32);
        _twoButtonFrame.activate();

        _blueButton.box().setSpriteSheet(&_blueBackgroundTexture);
        _blueButton.box().setCornerSize(32);
        _blueButton.label().setText("SecondButton");
        _blueButton.label().setTextSize(40);
        _blueButton.label().setTextColor(spk::Color(0, 100, 255));
        _blueButton.label().setOutlineSize(0);
        _blueButton.activate();
    }
};

int main()
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

    MainWidget mainWidget("MainWidget");
    mainWidget.setGeometry(0, app.size());
    mainWidget.activate();

    return (app.run());
}
