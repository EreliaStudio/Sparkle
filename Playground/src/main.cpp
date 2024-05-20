#include "sparkle.hpp"

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(640, 640), spk::Application::Mode::Multithread);

	spk::Font font("Playground/resources/font/Roboto-Regular.ttf");
	spk::SpriteSheet boxSpriteSheet("Playground/resources/texture/Frame1.png", spk::Vector2Int(3, 3));

	spk::TextLabel textLabel("TextLabelA", nullptr);
	textLabel.box().setSpriteSheet(&boxSpriteSheet);
	textLabel.box().setCornerSize(20);
	textLabel.label().setText("A");
	textLabel.label().setFont(&font);
	textLabel.label().setTextColor(spk::Color::white);
	textLabel.label().setOutlineColor(spk::Color::grey);

	std::cout << "Label size : "<< app.size() / 2 << std::endl;
	std::cout << "Label text renderer size : "<< (app.size() / 2) - textLabel.box().cornerSize() * 2 << std::endl;
	size_t textSize = font.computeOptimalTextSize(textLabel.label().text(), 0, (app.size() / 2) - textLabel.box().cornerSize() * 2);
	size_t outlineSize = static_cast<size_t>(textSize * 0.1f);
	textSize -= outlineSize * 2;

	textLabel.label().setTextSize(textSize);
	textLabel.label().setOutlineSize(outlineSize);
	textLabel.setGeometry(0, app.size() / 2);
	textLabel.activate();

	spk::TextLabel textLabel2("TextLabelB", nullptr);
	textLabel2.box().setCornerSize(20);
	textLabel2.box().setSpriteSheet(&boxSpriteSheet);
	textLabel2.label().setText("B");
	textLabel2.label().setFont(&font);
	textLabel2.label().setTextColor(spk::Color(50, 50, 50));
	textLabel2.label().setOutlineColor(spk::Color(220, 50, 15));

	textSize = font.computeOptimalTextSize(textLabel2.label().text(), 0, app.size() / 2 - textLabel2.box().cornerSize() * 2);
	outlineSize = static_cast<size_t>(textSize * 0.2f);
	textSize -= outlineSize * 2;

	textLabel2.label().setTextSize(textSize);
	textLabel2.label().setOutlineSize(outlineSize);
	textLabel2.setGeometry(app.size() / 2, app.size() / 2);
	textLabel2.activate();

    return (app.run());
}
