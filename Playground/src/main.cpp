#include "sparkle.hpp"

#include <set>

class FontRequester : public spk::Widget
{
private:
	spk::Font* _font;

	void _onUpdate() override
	{
		if (spk::Application::activeApplication()->keyboard().getChar() != L'\0')
		{
			const auto& glyph = _font->operator[](spk::Application::activeApplication()->keyboard().getChar()); 
		}
	}

	void _onGeometryChange() override
	{

	}

	void _onRender() override
	{
	}

public:
	FontRequester(spk::Font* p_font, spk::Widget* p_parent) :
		FontRequester(p_font, "Unnamed FontRequester", p_parent)
	{
	}

	FontRequester(spk::Font* p_font, const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_font(p_font)
	{
		
	}
};

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(640, 640), spk::Application::Mode::Multithread);

	spk::Font font("Playground/resources/font/Roboto-Regular.ttf");

	spk::ImageLabel fontTextureRenderer("FontTextureRenderer", nullptr);
	fontTextureRenderer.label().setTexture(&font);
	fontTextureRenderer.label().setTextureGeometry(spk::Vector2(0, 0), spk::Vector2(1, 1));
	fontTextureRenderer.setGeometry(0, app.size());
	fontTextureRenderer.setLayer(1);
	fontTextureRenderer.activate();

	FontRequester fontRequester(&font, "FontRequester", nullptr);
	fontRequester.activate();

    return (app.run());
}
