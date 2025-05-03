#include <sparkle.hpp>

int main()
{
	spk::GraphicalApplication app;

	spk::SafePointer<spk::Window> window = app.createWindow(L"Font Test", {{0, 0}, {800, 600}});
	spk::SafePointer<spk::Widget> root = window->widget();

	spk::TextLabel labelSmall = spk::TextLabel(L"LabelSmall", root);
	spk::TextLabel labelMedium = spk::TextLabel(L"LabelMedium", root);
	spk::TextLabel labelLarge = spk::TextLabel(L"LabelLarge", root);

	std::wstring sampleText = L"The quick brown fox jumps over the lazy dog";

	labelSmall.setText(sampleText);
	labelMedium.setText(sampleText);
	labelLarge.setText(sampleText);

	labelSmall.setTextColor(spk::Color::white, spk::Color::red);
	labelMedium.setTextColor(spk::Color::white, spk::Color::red);
	labelLarge.setTextColor(spk::Color::white, spk::Color::red);

	labelSmall.setFontSize({12, 4});
	labelMedium.setFontSize({24, 8});
	labelLarge.setFontSize({48, 16});

	labelSmall.setGeometry({50, 50}, {700, 50});
	labelMedium.setGeometry({50, 150}, {700, 100});
	labelLarge.setGeometry({50, 300}, {700, 150});

	labelSmall.activate();
	labelMedium.activate();
	labelLarge.activate();

	return app.run();
}