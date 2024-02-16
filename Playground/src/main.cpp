#include "sparkle.hpp"

#include "spk_button.hpp"
#include "spk_frame.hpp"
#include "spk_text_label.hpp"

class MainWidget : public spk::IWidget
{
private:
    spk::Font _textFont;
    spk::SpriteSheet _redBackgroundTexture;
    spk::SpriteSheet _lightGreenBackgroundTexture;
    spk::SpriteSheet _darkGreenBackgroundTexture;
    spk::SpriteSheet _blueBackgroundTexture;

    spk::Frame _listFrame;

    spk::Frame _twoButtonFrame;
    spk::Button _greenButtons[2];

    spk::Button _blueButton;


    void _onGeometryChange()
    {
        _listFrame.setGeometry(spk::Vector2Int(50, 0), spk::Vector2Int(300, size().y));

        spk::Vector2Int twoButtonFrameAnchor = spk::Vector2Int(_listFrame.anchor().x + _listFrame.size().x, 0);
        _twoButtonFrame.setGeometry(twoButtonFrameAnchor, spk::Vector2Int(size().x - twoButtonFrameAnchor.x, size().y / 2));

        spk::Vector2Int buttonHiddedAnchor = twoButtonFrameAnchor + spk::Vector2Int(0, _twoButtonFrame.size().y);
        spk::Vector2Int leftover = size() - buttonHiddedAnchor;
        _blueButton.setGeometry(buttonHiddedAnchor + leftover / 4, leftover / 2);

        spk::Vector2Int greenButtonAnchor = spk::Vector2Int(15, 15);
        spk::Vector2Int greenButtonSize = spk::Vector2Int(leftover.x - 30, (size().y / 2 - 45) / 2);
        for (size_t i = 0; i < 2; i++)
        {
            _greenButtons[i].setGeometry(greenButtonAnchor + (spk::Vector2Int(0, greenButtonSize.y + 15) * i), greenButtonSize);
        }
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
        _textFont("Playground/Roboto-Regular.ttf"),
        _listFrame("ListPanel"),
        _redBackgroundTexture("Playground/9SlicedRed.png", spk::Vector2Int(3, 3)),
        _twoButtonFrame("TwoButtonPanel"),
        _darkGreenBackgroundTexture("Playground/9SlicedDarkGreen.png", spk::Vector2Int(3, 3)),
        _blueButton("BlueButton", [&](){std::cout << "Blue button" << std::endl;}),
        _blueBackgroundTexture("Playground/9SlicedLightBlue.png", spk::Vector2Int(3, 3)),
        _greenButtons{
            spk::Button("GreenButtonA", [&](){std::cout << "Green button A" << std::endl;}),
            spk::Button("GreenButtonB", [&](){std::cout << "Green button B" << std::endl;})
        },
        _lightGreenBackgroundTexture("Playground/9SlicedLightGreen.png", spk::Vector2Int(3, 3))
    {
        _listFrame.box().setSpriteSheet(&_redBackgroundTexture);
        _listFrame.box().setCornerSize(32);
        _listFrame.activate();

        _blueButton.box().setSpriteSheet(&_blueBackgroundTexture);
        _blueButton.box().setCornerSize(32);
        _blueButton.label().setFont(&_textFont);
        _blueButton.label().setText("SecondButton");
        _blueButton.label().setTextSize(70);
        _blueButton.label().setTextColor(spk::Color(0, 100, 255));
        _blueButton.label().setOutlineSize(2);
        _blueButton.label().setOutlineColor(spk::Color(30, 30, 30));
        _blueButton.activate();

        _twoButtonFrame.box().setSpriteSheet(&_darkGreenBackgroundTexture);
        _twoButtonFrame.box().setCornerSize(32);
        _twoButtonFrame.activate();
        _twoButtonFrame.addChild(&(_greenButtons[0]));
        _twoButtonFrame.addChild(&(_greenButtons[1]));

        _greenButtons[0].box().setSpriteSheet(&_lightGreenBackgroundTexture);
        _greenButtons[0].box().setCornerSize(32);
        _greenButtons[0].label().setFont(&_textFont);
        _greenButtons[0].label().setText("GreenButtonA");
        _greenButtons[0].label().setTextSize(70);
        _greenButtons[0].label().setTextColor(spk::Color(0, 255, 100));
        _greenButtons[0].label().setOutlineSize(2);
        _greenButtons[0].label().setOutlineColor(spk::Color(30, 30, 30));
        _greenButtons[0].activate();

        _greenButtons[1].box().setSpriteSheet(&_lightGreenBackgroundTexture);
        _greenButtons[1].box().setCornerSize(32);
        _greenButtons[1].label().setFont(&_textFont);
        _greenButtons[1].label().setText("GreenButtonB");
        _greenButtons[1].label().setTextSize(70);
        _greenButtons[1].label().setTextColor(spk::Color(0, 255, 100));
        _greenButtons[1].label().setOutlineSize(2);
        _greenButtons[1].label().setOutlineColor(spk::Color(30, 30, 30));
        _greenButtons[1].activate();
    }
};

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 640), spk::Application::Mode::Monothread);

    MainWidget mainWidget("MainWidget");
    mainWidget.setGeometry(0, app.size());
    mainWidget.activate();

    return (app.run());
}