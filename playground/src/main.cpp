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

		// Add a simple square centered at (0,0), size 0.4
		float halfSize = 0.2f;
		colorRenderer << spk::ColorRenderer::Vertex{ spk::Vector2(-halfSize, -halfSize), 0.0f };
		colorRenderer << spk::ColorRenderer::Vertex{ spk::Vector2( halfSize, -halfSize), 0.0f };
		colorRenderer << spk::ColorRenderer::Vertex{ spk::Vector2(-halfSize,  halfSize), 0.0f };
		colorRenderer << spk::ColorRenderer::Vertex{ spk::Vector2( halfSize,  halfSize), 0.0f };

		std::array<unsigned int, 6> indices = { 0,1,2,2,1,3 };
		colorRenderer << indices;

		colorRenderer.validate();

		//-------------------------------------------
        // 2. Rendering text using FontRenderer, all at once
        //-------------------------------------------
        fontRenderer.clear();

        std::u32string text = U"Hello";
		
        spk::Vector2Int glyphAnchor = spk::Vector2Int(0, 0);

        auto& atlas = font.atlas({26,3});

		std::vector<unsigned int> indexes = {0, 1, 2, 2, 1, 3};

        unsigned int baseIndexes = 0; // Index offset for each glyph

        for (char32_t c : text)
        {
			const spk::Font::Glyph& glyph = atlas.glyph(c);

            for (size_t i = 0; i < 4; i++)
			{
				spk::FontRenderer::Vertex newVertex;

				newVertex.position = spk::Viewport::convertScreenToOpenGL(glyphAnchor + glyph.positions[i]).xy();
				newVertex.uv = glyph.UVs[i];
				newVertex.layer = 0.5f;

				fontRenderer << newVertex;
			}

			glyphAnchor += glyph.step;

			for (size_t i = 0; i < 6; i++)
			{
				fontRenderer << (baseIndexes + spk::Font::Glyph::indexesOrder[i]);
			}

			baseIndexes += 4;
        }

        fontRenderer.validate();

		//-------------------------------------------
        // 3. Rendering a textured quad using TextureRenderer
        //-------------------------------------------
        textureRenderer.clear();

        float imgHalfSize = 0.1f;
        float imgX = 0.0f;
        float imgY = 0.0f;

        std::array<spk::TextureRenderer::Vertex, 4> textureVertices = {
            spk::TextureRenderer::Vertex{{-0.1f, -0.1f}, 0.0f, {0.0f, 0.0f}},
            spk::TextureRenderer::Vertex{{+0.1f, -0.1f}, 0.0f, {1.0f, 0.0f}},
            spk::TextureRenderer::Vertex{{-0.1f, +0.1f}, 0.0f, {0.0f, 1.0f}},
            spk::TextureRenderer::Vertex{{+0.1f, +0.1f}, 0.0f, {1.0f, 1.0f}}
        };

        std::array<unsigned int, 6> textureIndices = {0,1,2,2,1,3};

        textureRenderer << textureVertices;
        textureRenderer << textureIndices;
        textureRenderer.validate();
	}

	void onPaintEvent(spk::PaintEvent& p_event)
	{
		if (_needUpdate == true)
		{
			_updateGPUData();
			_needUpdate = false;
		}
		//colorRenderer.render();
		fontRenderer.render();
        //textureRenderer.render();
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
	gameEngineWidget.setGeometry(win->geometry());
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();

	return (app.run());
}