#include "sparkle.hpp"

class Font
{
private:
    std::vector<uint8_t> _fontData;

public:
    Font(const std::filesystem::path& p_path)
    {
        std::ifstream file(p_path, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			spk::throwException("Failed to open font file [" + p_path.string() + "].");
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		_fontData.resize(static_cast<size_t>(size));
		if (!file.read(reinterpret_cast<char *>(_fontData.data()), size))
		{
			spk::throwException("Failed to read font file [" + p_path.string() + "].");
		}
    }
};

using Widget = spk::widget::IWidget;

class MainWidget : public Widget
{
private:
    Font _font;

    spk::widget::components::TextureRenderer _textureRenderer;

    void _onGeometryChange()
    {
        _textureRenderer.setGeometry(anchor(), size());
        _textureRenderer.setLayer(layer());
    }

    void _onRender()
    {
        _textureRenderer.render();
    }

public:
    MainWidget(const std::string& p_name) :
        Widget(p_name, nullptr),
        _font("Playground/resources/font/Roboto-Regular.ttf")
    {
        _textureRenderer.setTextureGeometry(0, 1);
    } 
};

int main() 
{
    spk::Application app = spk::Application("Labi", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    MainWidget mainWidget("ManiWidget");
    mainWidget.activate();

    return (app.run());  
}