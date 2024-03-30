#include "playground.hpp"

using Widget = spk::widget::IWidget;

class Button : public Widget
{
public:
    using Callback = std::function<void()>;

    enum class State
    {
        Pressed = 0,
        Released = 1
    };

private:
    State _state;
    spk::widget::components::NineSlicedBox _boxes[2];
    spk::widget::components::TextLabel _labels[2];

    Callback _onClickCallback;

    void _onGeometryChange()
    {
        for (size_t i = 0; i < 2; i++)
        {
            _boxes[i].setGeometry(anchor(), size());
            _boxes[i].setLayer(layer());

            _labels[i].setLayer(layer() + 0.01f);
            _labels[i].setAnchor(anchor() + size() / 2); 
        }

    }

    void _onRender() override
    {
        _boxes[static_cast<int>(_state)].render();
        _labels[static_cast<int>(_state)].render();
    }

    void _onUpdate() override
    {
        switch (_state)
        {
            case State::Released:
            {
                if (spk::Application::activeApplication()->mouse().getButton(spk::Mouse::Left) == spk::InputState::Down && 
                    hitTest(spk::Application::activeApplication()->mouse().position()) == true)
                {
                    _state = State::Pressed;
                    if (_onClickCallback != nullptr)
                        _onClickCallback();
                }
                break;
            }
            default:
            {
                if (spk::Application::activeApplication()->mouse().getButton(spk::Mouse::Left) == spk::InputState::Released ||
                    hitTest(spk::Application::activeApplication()->mouse().position()) == false)
                {
                    _state = State::Released;
                }
            }
        }
    }

public:
    Button(Widget* p_parent) :
        Widget(p_parent),
        _onClickCallback(nullptr)
    {
        _state = State::Released;
    }

    void setOnClickCallback(const Callback p_onClickCallback)
    {
        _onClickCallback = p_onClickCallback;
    }

    spk::widget::components::NineSlicedBox& box(const State& p_state)
    {
        return (_boxes[static_cast<int>(p_state)]);
    }

    spk::widget::components::TextLabel& label(const State& p_state)
    {
        return (_labels[static_cast<int>(p_state)]);
    }
}; 

class MainWidget : public Widget
{
private:
    spk::SpriteSheet _pressedTexture;
    spk::SpriteSheet _releasedTexture;
    spk::SpriteSheet _backgroundTexture;

    spk::Font _font;

    std::unique_ptr<Button> _myButton;

    std::unique_ptr<spk::widget::TextLabel> _myTextLabel;

public:
    MainWidget(const std::string& p_name) :
        Widget(p_name, nullptr),
        _pressedTexture("Playground/resources/texture/buttonPressed.png", spk::Vector2Int(3, 3)),
        _releasedTexture("Playground/resources/texture/buttonReleased.png", spk::Vector2Int(3, 3)),
        _backgroundTexture("Playground/resources/texture/buttonReleased.png", spk::Vector2Int(3, 3)),
        _font("Playground/resources/font/Heavitas.ttf")
    {
        auto offsetButtonLayout =  makeActiveChild<spk::widget::Offset>(100, this);
        auto sizeButtonLayout = offsetButtonLayout->makeActiveChild<spk::widget::SizedBox>(spk::Vector2Int(200, 100));
        _myButton = std::make_unique<Button>(sizeButtonLayout);

        _myButton->box(Button::State::Pressed).setSpriteSheet(&_pressedTexture);
        _myButton->box(Button::State::Released).setSpriteSheet(&_releasedTexture);

        _myButton->label(Button::State::Pressed).setFont(&_font);
        _myButton->label(Button::State::Pressed).setTextSize(25);
        _myButton->label(Button::State::Pressed).setTextColor(spk::Colors::white);
        _myButton->label(Button::State::Pressed).setOutlineSize(3);
        _myButton->label(Button::State::Pressed).setOutlineColor(spk::Colors::red);
        _myButton->label(Button::State::Pressed).setOutlineStyle(spk::Font::OutlineStyle::Standard);
        _myButton->label(Button::State::Pressed).setText("Pressed text");

        _myButton->label(Button::State::Released).setFont(&_font);
        _myButton->label(Button::State::Released).setTextSize(25);
        _myButton->label(Button::State::Released).setTextColor(spk::Colors::white);
        _myButton->label(Button::State::Released).setOutlineSize(3);
        _myButton->label(Button::State::Released).setOutlineColor(spk::Colors::red);
        _myButton->label(Button::State::Released).setOutlineStyle(spk::Font::OutlineStyle::Standard);
        _myButton->label(Button::State::Released).setText("Released text");

        _myButton->activate();
        
        auto offsetTextLabelLayout =  makeActiveChild<spk::widget::Offset>(100.0f, 300.0f, this);
        auto sizeTextLabelLayout = offsetTextLabelLayout->makeActiveChild<spk::widget::SizedBox>(spk::Vector2Int(200, 100));
        _myTextLabel = std::make_unique<spk::widget::TextLabel>(sizeTextLabelLayout);

        _myTextLabel->box().setSpriteSheet(&_releasedTexture);
        _myTextLabel->label().setText("Text label text");
        _myTextLabel->label().setFont(&_font);
        _myTextLabel->label().setTextSize(25);
        _myTextLabel->label().setTextColor(spk::Colors::white);
        _myTextLabel->label().setOutlineSize(3);
        _myTextLabel->label().setOutlineColor(spk::Colors::red);
        _myTextLabel->label().setOutlineStyle(spk::Font::OutlineStyle::Standard);

        _myTextLabel->activate();
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