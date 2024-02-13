#include "sparkle.hpp"

class MyWidget : public spk::IWidget
{
private:
    spk::WidgetComponent::NineSlicedBox _box;
    spk::WidgetComponent::TextLabel _label;

    void _onGeometryChange()
    {
        _box.setGeometry(anchor(), size());
        _box.setDepth(depth());

        _label.setAnchor(size() / 2);
        _label.setDepth(depth() + 1);
    }

    void _onRender()
    {
        _box.render();
        _label.render();
    }

    void _onUpdate()
    {
        
    }

public:
    MyWidget() : 
        IWidget("MyWidget")
    {
        spk::Font* myfont = new spk::Font("Chicken Market.ttf");

        _box.setCornerSize(32);
        _box.setSpriteSheet(new spk::SpriteSheet("backgroundBox.png", spk::Vector2Int(3, 3)));

        _label.setFont(myfont);
        _label.setText("!ABCDEFG");
        _label.setTextSize(100);
        _label.setTextColor(spk::Color(255, 255, 255));
        _label.setOutlineSize(0);
        _label.setOutlineStyle(spk::Font::OutlineStyle::Standard);
        _label.setOutlineColor(spk::Color(120, 120, 120));
        _label.setVerticalAlignment(spk::VerticalAlignment::Centered);
        _label.setHorizontalAlignment(spk::HorizontalAlignment::Centered);
    }
};

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    MyWidget myWidget;
    myWidget.setGeometry(0, app.size());
    myWidget.activate();

    return (app.run());
}