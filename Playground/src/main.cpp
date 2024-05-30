#include "sparkle.hpp"

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(640, 640), spk::Application::Mode::Multithread);

	spk::Font font("Playground/resources/font/Roboto-Regular.ttf");
	spk::SpriteSheet boxSpriteSheet("Playground/resources/texture/Frame1.png", spk::Vector2Int(3, 3));


	for (size_t i = 0; i < 8; i++)
	{
		spk::TextLabel* textLabel = new spk::TextLabel("TextLabel", nullptr);
		textLabel->box().setSpriteSheet(&boxSpriteSheet);
		textLabel->box().setCornerSize(20);
		textLabel->label().setText("A");
		textLabel->label().setFont(&font);
		textLabel->label().setTextColor(spk::Color::white);
		textLabel->label().setTextEdgeStrenght(20.0f);
		textLabel->label().setOutlineEdgeStrenght(20.0f);
		textLabel->label().setOutlineColor(spk::Color::black);
		textLabel->label().setVerticalAlignment(spk::VerticalAlignment::Centered);
		textLabel->label().setHorizontalAlignment(spk::HorizontalAlignment::Centered);
		textLabel->label().setFontSize(spk::Font::Size(20, i * 2));
		textLabel->setGeometry(app.size() / spk::Vector2(1, 8) * spk::Vector2Int(0, i), app.size() / spk::Vector2(1, 8));
		textLabel->activate();
	}

    return (app.run());
}
