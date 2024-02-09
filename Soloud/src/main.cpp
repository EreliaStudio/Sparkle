#include <iostream>

#include "sparkle.hpp"
#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_thread.h"

const int WWIDTH = 1000;
const int WHEIGHT = 1000;

class Positioned : public spk::IWidget
{
public:
    Positioned(spk::Vector2 p_pos, IWidget *p_parent = nullptr) : IWidget("PositionedInstance", p_parent), _pos(p_pos)
    {
    }

private:
    void _onGeometryChange() override
    {
        for (auto *child : childrens())
        {
            child->setGeometry(anchor() + _pos, size());
        }
    }

    void _onRender() override
    {
    }

    void _onUpdate() override
    {
    }
    spk::Vector2 _pos;
};

class SizedBox : public spk::IWidget
{
public:
    SizedBox(const spk::Vector2 &p_size, IWidget *p_parent = nullptr) : IWidget("SizedBoxInstance", p_parent), _size(p_size)
    {
        setGeometry(anchor(), _size);
    }

private:
    void _onGeometryChange() override
    {
        for (auto *child : childrens())
        {
            child->setGeometry(anchor(), _size);
        }
    }

    void _onRender() override
    {
    }

    void _onUpdate() override
    {
    }

    spk::Vector2 _size;
};

class TextWidget : public spk::IWidget
{
public:
    TextWidget(const std::string &name, const std::string &text, spk::Font *font, IWidget *parent = nullptr)
        : IWidget(name, parent), _text(text), _font(font), _textLabel()
    {
        _textLabel.setFont(_font);
        _textLabel.setText(_text);
        _textLabel.setTextSize(80);
        _textLabel.setTextColor(spk::Color{255, 255, 0, 200});
        _textLabel.setVerticalAlignment(spk::VerticalAlignment::Top);
        _textLabel.setHorizontalAlignment(spk::HorizontalAlignment::Left);
    }

    TextWidget(const std::string &text, spk::Font *font, IWidget *parent = nullptr)
        : TextWidget("TextWidgetInstance", text, font, parent) {}

private:
    void _onGeometryChange() override
    {
        _textLabel.setAnchor(anchor());
    }

    void _onRender() override
    {
        _textLabel.render();
    }

    void _onUpdate() override
    {
    }

    std::string _text;
    spk::Font *_font;
    spk::WidgetComponent::TextLabel _textLabel;
};

int main()
{
    spk::Font honk{"C:/Users/JQ/dev/Fonts/NotoSans/static/NotoSans-Regular.ttf"};

    spk::Application app = spk::Application("sound test", spk::Vector2UInt{WWIDTH, WHEIGHT});

    spk::GameEngine engine;
    spk::GameEngineManager manager{"GameView"};

    manager.setGameEngine(&engine);
    manager.activate();

    TextWidget a{"A", &honk};
    a.activate();

    Positioned pb{{200, 200}};
    SizedBox sb{{200, 200}, &pb};
    TextWidget b{"B", &honk, &sb};

    SoLoud::Soloud gSoloud;
    SoLoud::Wav gWav;

    gSoloud.init();
    if (SoLoud::SO_NO_ERROR != gWav.load("C:/Users/JQ/Music/sound_effects/applause_y.wav")) {
        return 1;
    }
    std::cout << gWav.getLength() << std::endl;
    int handle = gSoloud.play(gWav);
    std::cout << handle << std::endl;


    int errorCode = app.run();
    gSoloud.deinit();
    std::cout<< "Finished" << std::endl;
    return errorCode;
}