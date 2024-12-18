#include "playground.hpp"

class TestComponent : public spk::Component
{
private:
	bool _needUpdate = true;

	spk::ColorRenderer colorRenderer;
    spk::FontRenderer fontRenderer;
    spk::TextureRenderer textureRenderer;
	
	spk::Font font;
    spk::Image image;

public:
	TestComponent() :
		spk::Component(L"TestComponent"),
		font("playground/resources/CrystalUniverse-Regular.ttf"),
		image("playground/resources/test.png")
	{
		fontRenderer.setFont(&font);
        fontRenderer.setFontSize({26, 3});
        fontRenderer.setGlyphColor(spk::Color(255, 255, 255, 255));
        fontRenderer.setOutlineColor(spk::Color(255, 0, 0, 255));

		textureRenderer.setTexture(&image);
	}

	void _updateGPUData()
	{
		//-------------------------------------------
        // 1. Example usage of the ColorRenderer (already given)
        //-------------------------------------------
		colorRenderer.clear();

		colorRenderer.prepareSquare({{0, 0}, {100, 100}}, 0.0f);

		colorRenderer.validate();

		//-------------------------------------------
        // 2. Rendering text using FontRenderer, all at once
        //-------------------------------------------
        fontRenderer.clear();

        fontRenderer.prepare(L"Hello world", spk::Vector2Int(200, 50), 2.0f);

        fontRenderer.validate();

		//-------------------------------------------
        // 3. Rendering a textured quad using TextureRenderer
        //-------------------------------------------
        textureRenderer.clear();

		textureRenderer.prepare({{100, 100}, {100, 100}}, spk::Image::Section(0, 1), 1.0f);

        textureRenderer.validate();
	}

	void onPaintEvent(spk::PaintEvent& p_event) override
	{
		if (p_event.resized == true)
		{
			_updateGPUData();
		}
		colorRenderer.render();
		fontRenderer.render();
        textureRenderer.render();
	}

};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {840, 680}});

	spk::GameEngine engine;

	spk::Entity player(L"Player");
	player.addComponent<TestComponent>();
	
	spk::Entity camera(L"Camera", &player);

	spk::Entity cube(L"Cube");

	engine.addEntity(&player);
	engine.addEntity(&cube);

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setGeometry(win->geometry().anchor, win->geometry().size);
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();

	return (app.run());
}