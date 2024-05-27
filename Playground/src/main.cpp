#include "sparkle.hpp"

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(640, 640), spk::Application::Mode::Multithread);

	spk::Font font("Playground/resources/font/Roboto-Regular.ttf");
	spk::SpriteSheet boxSpriteSheet("Playground/resources/texture/Frame1.png", spk::Vector2Int(3, 3));

	spk::TextLabel textLabel("TextLabelA", nullptr);
	textLabel.box().setSpriteSheet(&boxSpriteSheet);
	textLabel.box().setCornerSize(20);
	textLabel.label().setText("W");
	textLabel.label().setFont(&font);
	textLabel.label().setTextColor(spk::Color::white);
	textLabel.label().setTextEdgeStrenght(10.0f);
	textLabel.label().setOutlineEdgeStrenght(0.15f);
	textLabel.label().setOutlineColor(spk::Color::black);
	textLabel.label().setVerticalAlignment(spk::VerticalAlignment::Centered);
	textLabel.label().setHorizontalAlignment(spk::HorizontalAlignment::Centered);

	auto fontSize = font.computeOptimalTextSize(textLabel.label().text(), 0.08f, (app.size() / 2) - textLabel.box().cornerSize() * 2);

	textLabel.label().setTextSize(std::get<0>(fontSize));
	textLabel.label().setOutlineSize(std::get<1>(fontSize));
	textLabel.setGeometry(0, app.size() / 2);
	textLabel.activate();

	spk::TextLabel textLabelB("TextLabelB", nullptr);
	textLabelB.box().setSpriteSheet(&boxSpriteSheet);
	textLabelB.box().setCornerSize(20);
	textLabelB.label().setText("Ceci est un test");
	textLabelB.label().setFont(&font);
	textLabelB.label().setTextColor(spk::Color::white);
	textLabelB.label().setTextEdgeStrenght(10.0f);
	textLabelB.label().setOutlineEdgeStrenght(0.15f);
	textLabelB.label().setOutlineColor(spk::Color::black);
	textLabelB.label().setVerticalAlignment(spk::VerticalAlignment::Centered);
	textLabelB.label().setHorizontalAlignment(spk::HorizontalAlignment::Centered);

	fontSize = font.computeOptimalTextSize(textLabelB.label().text(), 0.08f, (app.size() / 2) - textLabelB.box().cornerSize() * 2);

	textLabelB.label().setTextSize(std::get<0>(fontSize));
	textLabelB.label().setOutlineSize(std::get<1>(fontSize));
	textLabelB.setGeometry(app.size() / 2, app.size() / 2);
	textLabelB.activate();

    return (app.run());
}
