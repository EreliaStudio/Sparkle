#include "sparkle.hpp"

class MyButton : public spk::Button
{
private:
    static inline std::unique_ptr<spk::SpriteSheet> _defaultPressedButtonSpriteSheet = nullptr;
    static inline std::unique_ptr<spk::SpriteSheet> _defaultReleasedButtonSpriteSheet = nullptr;
    static inline std::unique_ptr<spk::Font> _defaultButtonFont = nullptr;

public:
    MyButton(Widget* p_parent = nullptr) :
        spk::Button(p_parent)
    {
        if (_defaultPressedButtonSpriteSheet == nullptr)
            _defaultPressedButtonSpriteSheet = std::make_unique<spk::SpriteSheet>("Playground/resources/texture/buttonPressed.png", spk::Vector2Int(3, 3));

        if (_defaultReleasedButtonSpriteSheet == nullptr)
            _defaultReleasedButtonSpriteSheet = std::make_unique<spk::SpriteSheet>("Playground/resources/texture/buttonReleased.png", spk::Vector2Int(3, 3));

        if (_defaultButtonFont == nullptr)
            _defaultButtonFont = std::make_unique<spk::Font>("Playground/resources/font/Roboto-Regular.ttf");

        box(spk::Button::State::Pressed).setSpriteSheet(_defaultPressedButtonSpriteSheet.get());
        box(spk::Button::State::Pressed).setCornerSize(6);
        box(spk::Button::State::Released).setSpriteSheet(_defaultReleasedButtonSpriteSheet.get());
        box(spk::Button::State::Released).setCornerSize(6);

        label(spk::Button::State::Pressed).setFont(_defaultButtonFont.get());
        label(spk::Button::State::Pressed).setTextSize(25);
        label(spk::Button::State::Pressed).setTextColor(spk::Color::white);
        label(spk::Button::State::Pressed).setOutlineSize(1);
        label(spk::Button::State::Pressed).setOutlineColor(spk::Color::black);
        label(spk::Button::State::Pressed).setOutlineStyle(spk::Font::OutlineStyle::Standard);
        label(spk::Button::State::Pressed).setHorizontalAlignment(spk::HorizontalAlignment::Centered);
        label(spk::Button::State::Pressed).setVerticalAlignment(spk::VerticalAlignment::Centered);

        label(spk::Button::State::Released).setFont(_defaultButtonFont.get());
        label(spk::Button::State::Released).setTextSize(25);
        label(spk::Button::State::Released).setTextColor(spk::Color::white);
        label(spk::Button::State::Released).setOutlineSize(1);
        label(spk::Button::State::Released).setOutlineColor(spk::Color::black);
        label(spk::Button::State::Released).setOutlineStyle(spk::Font::OutlineStyle::Standard);
        label(spk::Button::State::Released).setHorizontalAlignment(spk::HorizontalAlignment::Centered);
        label(spk::Button::State::Released).setVerticalAlignment(spk::VerticalAlignment::Centered);
    }
};

class MainWidget : public spk::Widget
{
private:
    MyButton _buttonA;
    MyButton _buttonB;

    void _onGeometryChange()
    {
        _buttonA.setGeometry(spk::Vector2Int(400, 100), spk::Vector2Int(300, 100));
        _buttonB.setGeometry(spk::Vector2Int(400, 300), spk::Vector2Int(300, 100));
    }

    void _onRender()
    {   

    }

public:
    MainWidget(const std::string& p_name) :
        spk::Widget(p_name, nullptr),
        _buttonA(this),
        _buttonB(this)
    {
        _buttonA.label(spk::Button::State::Pressed).setText("I am clicked");
        _buttonA.label(spk::Button::State::Released).setText("Click me");

        _buttonA.activate();

        _buttonB.label(spk::Button::State::Pressed).setText("I have been clicked !");
        _buttonB.label(spk::Button::State::Released).setText("Click on me also");

        _buttonB.activate();
    } 
};

int main() 
{
    spk::Application app = spk::Application("Labi", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    MainWidget mainWidget("ManiWidget");
    mainWidget.setGeometry(0, app.size());
    mainWidget.activate();

    return (app.run());  
}