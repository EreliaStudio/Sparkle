#include "sparkle.hpp"

class MainWidget : public spk::Widget
{
private:
    spk::WidgetComponent::NineSlicedBox _nineSlice;

    void _onGeometryChange()
    {
        _nineSlice.setGeometry(anchor(), size());
        _nineSlice.setLayer(layer());
    }

    void _onRender()
    {
        _nineSlice.render();
    }

public:
    MainWidget(const std::string& p_name) :
        spk::Widget(p_name, nullptr)
    {
        _nineSlice.setSpriteSheet(new spk::SpriteSheet("Playground/resources/texture/buttonReleased.png", spk::Vector2Int(3, 3)));
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