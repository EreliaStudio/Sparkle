#include "sparkle.hpp"

using Widget = spk::widget::IWidget;

class FontRenderer : public Widget
{
private:
    spk::widget::components::TextureRenderer _textureRenderer;

    void _onGeometryChange()
    {
        _textureRenderer.setGeometry(anchor(), size());
        if (false)
        {
            _textureRenderer.setTextureGeometry(spk::Vector2(0, 0), spk::Vector2(1.0f, 1.0f));
        }
        else
        {
            float ratio = static_cast<float>(size().x) / static_cast<float>(size().y);
            float value = 0.1f;
            _textureRenderer.setTextureGeometry(spk::Vector2(0, 0), spk::Vector2(value * ratio, value));
        }
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

    }

    void setFont(spk::Font* p_font)
    {
        _textureRenderer.setTexture(&(p_font->atlas(50, 10, spk::Font::OutlineStyle::Standard).texture()));
    }
};

class MainWidget : public Widget
{
private:
    std::unique_ptr<spk::Font> _myFont;
    std::unique_ptr<FontRenderer> _myFontRenderer;

    std::unique_ptr<spk::widget::TextLabel> _myTextLabel;

public:
    MainWidget(const std::string& p_name) :
        Widget(p_name, nullptr)
    { 
        _myFont = std::make_unique<spk::Font>("Playground/resources/font/Heavitas.ttf");

        auto paddingParent = makeActiveChild<spk::widget::Offset>(0.0f, 0.0f);
        auto sizeParent = paddingParent->makeActiveChild<spk::widget::FractionallySizedBox>(1.0f, 0.5f);

        // _myFontRenderer = std::make_unique<FontRenderer>(sizeParent);
        // _myFontRenderer->setFont(_myFont);
        // _myFontRenderer->activate();    

        _myTextLabel = std::make_unique<spk::widget::TextLabel>(sizeParent);

        _myTextLabel->label().setFont(_myFont.get());
        _myTextLabel->label().setText("Ceci est un test");
        _myTextLabel->label().setTextColor(spk::Colors::white);
        _myTextLabel->label().setTextSize(50);
        _myTextLabel->label().setOutlineColor(spk::Colors::red);
        _myTextLabel->label().setOutlineSize(10);
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