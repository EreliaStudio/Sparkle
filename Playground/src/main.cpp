#include "sparkle.hpp"

using Widget = spk::widget::IWidget;

class FontRenderer : public Widget
{
private:
    std::unique_ptr<spk::Font> _myFont;
    spk::widget::components::TextureRenderer _textureRenderer;

    void _onGeometryChange()
    {
        _textureRenderer.setGeometry(anchor(), size());
        float ratio = static_cast<float>(size().x) / static_cast<float>(size().y);
        _textureRenderer.setTextureGeometry(spk::Vector2(0, 0), spk::Vector2(0.05f * ratio, 0.05f));
        _textureRenderer.setLayer(layer());
    }

    void _onRender()
    {
        _textureRenderer.render();
    }

public:
    FontRenderer(Widget* p_parent = nullptr) :
        Widget(p_parent)
    {
        _myFont = std::make_unique<spk::Font>("Playground/resources/font/Heavitas.ttf");

        _textureRenderer.setTexture(&(_myFont->atlas(50, 10, spk::Font::OutlineStyle::Standard).texture()));
    }
};

class MainWidget : public Widget
{
private:
    std::unique_ptr<FontRenderer> _myFontRenderer;

public:
    MainWidget(const std::string& p_name) :
        Widget(p_name, nullptr)
    { 
        auto paddingParent = makeActiveChild<spk::widget::Padding>(0.0f, 0.0f, 0.0f, 0.0f);
        auto sizeParent = paddingParent->makeActiveChild<spk::widget::FractionallySizedBox>(1.0f, 1.0f);

        _myFontRenderer = std::make_unique<FontRenderer>(sizeParent);
        _myFontRenderer->activate();    
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