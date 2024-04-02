#include "sparkle.hpp"

class MainWidget : public spk::Widget
{
private:
    spk::SpriteSheet _pressedTexture;
    spk::SpriteSheet _releasedTexture;
    spk::Font _font;

    spk::Button _buttonA;
    spk::Button _buttonB;

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
        _buttonB(this),
        _pressedTexture("Playground/resources/texture/buttonPressed.png", spk::Vector2Int(3, 3)),
        _releasedTexture("Playground/resources/texture/buttonReleased.png", spk::Vector2Int(3, 3)),
        _font("Playground/resources/font/Roboto-Regular.ttf")
    {
        _buttonA.box(spk::Button::State::Pressed).setSpriteSheet(&_pressedTexture);
        _buttonA.box(spk::Button::State::Released).setSpriteSheet(&_releasedTexture);

        _buttonA.label(spk::Button::State::Pressed).setFont(&_font);
        _buttonA.label(spk::Button::State::Pressed).setTextSize(25);
        _buttonA.label(spk::Button::State::Pressed).setTextColor(spk::Color::white);
        _buttonA.label(spk::Button::State::Pressed).setOutlineSize(3);
        _buttonA.label(spk::Button::State::Pressed).setOutlineColor(spk::Color::red);
        _buttonA.label(spk::Button::State::Pressed).setOutlineStyle(spk::Font::OutlineStyle::Standard);
        _buttonA.label(spk::Button::State::Pressed).setHorizontalAlignment(spk::HorizontalAlignment::Left);
        _buttonA.label(spk::Button::State::Pressed).setVerticalAlignment(spk::VerticalAlignment::Top);
        _buttonA.label(spk::Button::State::Pressed).setText("A");

        _buttonA.label(spk::Button::State::Released).setFont(&_font);
        _buttonA.label(spk::Button::State::Released).setTextSize(25);
        _buttonA.label(spk::Button::State::Released).setTextColor(spk::Color::white);
        _buttonA.label(spk::Button::State::Released).setOutlineSize(3);
        _buttonA.label(spk::Button::State::Released).setOutlineColor(spk::Color::red);
        _buttonA.label(spk::Button::State::Released).setOutlineStyle(spk::Font::OutlineStyle::Standard);
        _buttonA.label(spk::Button::State::Released).setHorizontalAlignment(spk::HorizontalAlignment::Right);
        _buttonA.label(spk::Button::State::Released).setVerticalAlignment(spk::VerticalAlignment::Down);
        _buttonA.label(spk::Button::State::Released).setText("A");

        _buttonA.activate();

        
        _buttonB.box(spk::Button::State::Pressed).setSpriteSheet(&_pressedTexture);
        _buttonB.box(spk::Button::State::Released).setSpriteSheet(&_releasedTexture);

        _buttonB.label(spk::Button::State::Pressed).setFont(&_font);
        _buttonB.label(spk::Button::State::Pressed).setTextSize(25);
        _buttonB.label(spk::Button::State::Pressed).setTextColor(spk::Color::white);
        _buttonB.label(spk::Button::State::Pressed).setOutlineSize(3);
        _buttonB.label(spk::Button::State::Pressed).setOutlineColor(spk::Color::red);
        _buttonB.label(spk::Button::State::Pressed).setOutlineStyle(spk::Font::OutlineStyle::Standard);
        _buttonB.label(spk::Button::State::Pressed).setHorizontalAlignment(spk::HorizontalAlignment::Left);
        _buttonB.label(spk::Button::State::Pressed).setVerticalAlignment(spk::VerticalAlignment::Top);
        _buttonB.label(spk::Button::State::Pressed).setText("A");

        _buttonB.label(spk::Button::State::Released).setFont(&_font);
        _buttonB.label(spk::Button::State::Released).setTextSize(25);
        _buttonB.label(spk::Button::State::Released).setTextColor(spk::Color::white);
        _buttonB.label(spk::Button::State::Released).setOutlineSize(3);
        _buttonB.label(spk::Button::State::Released).setOutlineColor(spk::Color::red);
        _buttonB.label(spk::Button::State::Released).setOutlineStyle(spk::Font::OutlineStyle::Standard);
        _buttonB.label(spk::Button::State::Released).setHorizontalAlignment(spk::HorizontalAlignment::Right);
        _buttonB.label(spk::Button::State::Released).setVerticalAlignment(spk::VerticalAlignment::Down);
        _buttonB.label(spk::Button::State::Released).setText("A");

        //_buttonB.activate();
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