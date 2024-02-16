#include "sparkle.hpp"

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
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    MainWidget mainWidget("MainWidget");
    mainWidget.setGeometry(0, app.size());
    mainWidget.activate();

    return (app.run());
}