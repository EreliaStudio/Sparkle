#include "sparkle.hpp"

using Widget = spk::widget::IWidget;

class Offset : public spk::widget::SingleChildWidget
{
private:
    spk::Vector2Int _childAnchor;

    spk::Vector2 layout(const spk::widget::BoxConstraints& p_constraints) override
    {
        IWidget* child = SingleChildWidget::child();
        if (nullptr != child)
        {
            spk::Vector2 max = p_constraints.max - _childAnchor;
            spk::Vector2 min = p_constraints.min - _childAnchor;
            spk::Vector2 childSize = child->layout({min, max});

            child->setGeometry(_childAnchor, childSize);
            return childSize + _childAnchor;
        }

        return p_constraints.max;
    }

public:
    Offset(const spk::Vector2Int& p_childAnchor, spk::widget::IWidget* p_parent = nullptr) : 
        spk::widget::SingleChildWidget(p_parent),
        _childAnchor(p_childAnchor)
    {

    }
};

class Button : public Widget
{
public:
    using Callback = std::function<void()>;
private:
    static inline spk::SpriteSheet* _pressedDefaultTexture = nullptr;
    static inline spk::SpriteSheet* _releasedDefaultTexture = nullptr;
    static inline spk::Font* _labelDefaultFont = nullptr;

    int _boxIndex = 0;
    spk::widget::components::NineSlicedBox _boxes[2];
    spk::widget::components::TextLabel _label;

    Callback _onClickCallback;

    void _onGeometryChange()
    {
        for (size_t i = 0; i < 2; i++)
        {
            _boxes[i].setGeometry(anchor(), size());
            _boxes[i].setDepth(depth());
        }

        _label.setDepth(depth() - 0.01f);
        _label.setAnchor(anchor() + size() / 2); 
    }

    void _onRender() override
    {
        _boxes[_boxIndex].render();
        _label.render();
    }

    void _onUpdate() override
    {
        if (hitTest(spk::Application::activeApplication()->mouse().position()) == true)
        {
            _boxIndex = 1;
        }
        else
            _boxIndex = 0;
    }

public:
    Button(const std::string& p_buttonText, const Callback& p_onClickCallback, Widget* p_parent) :
        Widget(p_parent),
        _onClickCallback(p_onClickCallback)
    {
        if (_pressedDefaultTexture == nullptr)
            _pressedDefaultTexture = new spk::SpriteSheet("resources/texture/buttonPressed.png", spk::Vector2Int(3, 3));
        if (_releasedDefaultTexture == nullptr)
            _releasedDefaultTexture = new spk::SpriteSheet("resources/texture/buttonReleased.png", spk::Vector2Int(3, 3));
        if (_labelDefaultFont == nullptr)
            _labelDefaultFont = new spk::Font("resources/font/Roboto-Regular.ttf");

        _boxIndex = 0;

        _boxes[0].setSpriteSheet(_releasedDefaultTexture);
        _boxes[0].setCornerSize(6);

        _boxes[1].setSpriteSheet(_pressedDefaultTexture);
        _boxes[1].setCornerSize(6);

        _label.setText(p_buttonText);
        _label.setTextSize(25); 
        _label.setTextColor(spk::Colors::white);
        _label.setFont(_labelDefaultFont);
        _label.setHorizontalAlignment(spk::HorizontalAlignment::Centered);
        _label.setVerticalAlignment(spk::VerticalAlignment::Centered);
        _label.setOutlineSize(2);
        _label.setOutlineStyle(spk::Font::OutlineStyle::Standard);
        _label.setOutlineColor(spk::Colors::black);
    }
};

class MainWidget : public Widget
{
private:
    std::unique_ptr<Button> _myButton;

public:
    MainWidget(const std::string& p_name) :
        Widget(p_name, nullptr)
    { 
        auto offsetButtonLayout =  makeActiveChild<Offset>(100, this);
        auto sizeButtonLayout = offsetButtonLayout->makeActiveChild<spk::widget::SizedBox>(spk::Vector2Int(200, 100));
        _myButton = std::make_unique<Button>("Click Me", [&](){std::cout << "Button pressed" << std::endl;}, sizeButtonLayout);

        _myButton->activate();
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
