#include "sparkle.hpp"

class Frame : public spk::IWidget
{
private:
    spk::WidgetComponent::NineSlicedBox _box;

    void _onGeometryChange()
    {
        _box.setDepth(depth());
        _box.setGeometry(anchor(), size());
    }

    void _onRender()
    {
        _box.render();
    }

    void _onUpdate()
    {

    }

public:
    Frame(const std::string& p_name, IWidget* p_parent = nullptr) : 
        IWidget(p_name)
    {
        _box.setCornerSize(spk::Vector2Int(32, 32));
    }

    void setSpriteSheet(spk::SpriteSheet* p_spriteSheet)
    {
        _box.setSpriteSheet(p_spriteSheet);
    }
};

class TextLabel : public spk::IWidget
{
public:

private:
    spk::WidgetComponent::NineSlicedBox _box;

    spk::VerticalAlignment _verticalTextPosition;
    spk::HorizontalAlignment _horizontalTextPosition;

    spk::WidgetComponent::TextLabel _label;

    void _onGeometryChange()
    {
        _box.setDepth(depth());
        _box.setGeometry(anchor(), size());

        _label.setAnchor(anchor() + size() / 2);
        _label.setTextSize(std::max((size().y / 20), 70u));
        _label.setOutlineSize(std::max((size().y / 40), 5u));
        _label.setDepth(depth() + 0.01f);
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
    TextLabel(const std::string& p_text, const std::string& p_name, IWidget* p_parent = nullptr) : 
        IWidget(p_name)
    {
        _box.setCornerSize(32);

        _label.setText(p_text);
        _label.setTextColor(spk::Color(255, 255, 255));
        _label.setOutlineColor(spk::Color(50, 50, 50));
        _label.setOutlineStyle(spk::Font::OutlineStyle::Standard);
        _label.setVerticalAlignment(spk::VerticalAlignment::Centred);
        _label.setHorizontalAlignment(spk::HorizontalAlignment::Centred);
    }

    void setBackgroundSheet(spk::SpriteSheet* p_spriteSheet)
    {
        _box.setSpriteSheet(p_spriteSheet);
    }

    void setFont(spk::Font* p_font)
    {
        _label.setFont(p_font);
    }

    void setTextPosition(const spk::VerticalAlignment& p_verticalPosition, const spk::HorizontalAlignment& p_horizontalPosition)
    {
        _verticalTextPosition = p_verticalPosition;
        _horizontalTextPosition = p_horizontalPosition;
    }
};

class MainWidget : public spk::IWidget
{
private:
    spk::SpriteSheet* _frameSpriteSheet;
    spk::Font* _textLabelFont;

    Frame _backgroundFrame;
    std::vector<TextLabel> _indicativeTextLabels;

    void _onGeometryChange()
    {
        _backgroundFrame.setGeometry(anchor(), size());

        spk::Vector2Int labelSize = spk::Vector2Int(500, 200);
        for (size_t i = 0; i < _indicativeTextLabels.size(); i++)
        {
            _indicativeTextLabels[i].setGeometry(spk::Vector2Int((size().x - labelSize.x) / 2, 0) + labelSize * i * spk::Vector2Int(0, 1), labelSize);
        }
    }

    void _onRender()
    {

    }

    void _onUpdate()
    {

    }

public:
    MainWidget() : 
        IWidget("MainWidget"),
        _backgroundFrame("backgroundFrame", this),
        _frameSpriteSheet(new spk::SpriteSheet("boxTextureExemple.png", spk::Vector2UInt(3, 3))),
        _textLabelFont(new spk::Font("LTPanneaux-Regular.ttf"))
    {
        _backgroundFrame.setSpriteSheet(_frameSpriteSheet);
        _backgroundFrame.setDepth(10);
        _backgroundFrame.activate();

        for (size_t i = 0; i < 10; i++)
        {
            TextLabel newLabel("Label [" + std::to_string(i) + "]", "Label" + std::to_string(i), this);

            newLabel.setBackgroundSheet(_frameSpriteSheet);
            newLabel.setTextPosition(spk::VerticalAlignment::Top, spk::HorizontalAlignment::Centred);
            newLabel.setFont(_textLabelFont);
            newLabel.setDepth(100);
            newLabel.activate();

            _indicativeTextLabels.emplace_back(newLabel);
        }
    }
};

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800));

    MainWidget mainWidget;
    mainWidget.setGeometry(0, spk::Application::activeApplication()->size());
    //mainWidget.activate();

    return app.run();
}