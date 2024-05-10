#include "sparkle.hpp"

int main() 
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    spk::TextEntry entry;
    entry.setGeometry(100, spk::Vector2Int(150, 50));
    entry.activate();

    entry.box().setSpriteSheet(new spk::SpriteSheet("Playground/resources/texture/buttonPressed.png", spk::Vector2Int(3, 3)));
    entry.box().setCornerSize(2);

    entry.label().setFont(new spk::Font("Playground/resources/font/Heavitas.ttf"));
    entry.label().setHorizontalAlignment(spk::HorizontalAlignment::Left);
    entry.label().setTextColor(spk::Color(255, 255, 255));
    entry.label().setTextSize(30);
    entry.label().setOutlineColor(spk::Color(50, 50, 50));
    entry.label().setOutlineSize(2);
    entry.label().setText("Ceci est un test");

    return (app.run());  
}