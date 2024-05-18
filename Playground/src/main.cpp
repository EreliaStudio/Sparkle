#include "sparkle.hpp"

class CustomTextureManager : public spk::TextureAtlas
{
private:
	std::map<std::string, spk::Font*> _fonts;

public:
	CustomTextureManager();
	~CustomTextureManager();

	spk::Font* loadFont(const std::string& p_fontName, const std::filesystem::path& p_fontPath);
	spk::Font* font(const std::string& p_fontName);
};

using TextureAtlas = spk::Singleton<CustomTextureManager>;

class Frame : public spk::Frame
{
private:

public:
	Frame(Widget* p_parent) :
		Frame("Anonymous Frame", p_parent)
	{

	}
	Frame(const std::string& p_name, Widget* p_parent) :
		spk::Frame(p_name, p_parent)
	{
		box().setSpriteSheet(TextureAtlas::instance()->spriteSheet("DefaultFrame"));
		box().setCornerSize(8);
	}
};

class TextLabel : public spk::TextLabel
{
private:

public:
	TextLabel(Widget* p_parent) :
		TextLabel("Anonymous TextLabel", p_parent)
	{

	}
	TextLabel(const std::string& p_name, Widget* p_parent) :
		spk::TextLabel(p_name, p_parent)
	{
		box().setSpriteSheet(TextureAtlas::instance()->spriteSheet("DefaultFrame"));
		box().setCornerSize(8);

		label().setFont(TextureAtlas::instance()->font("DefaultFont"));
		label().setVerticalAlignment(spk::VerticalAlignment::Top);
		label().setHorizontalAlignment(spk::HorizontalAlignment::Left);

		label().setTextSize(25);
		label().setTextColor(spk::Color(255, 255, 255));

		label().setOutlineSize(3);
		label().setOutlineColor(spk::Color(0, 0, 0));
		label().setOutlineStyle(spk::Font::OutlineStyle::Standard);
	}
};

class TextEntry : public spk::TextEntry
{
private:

public:
	TextEntry(Widget* p_parent) :
		TextEntry("Anonymous TextEntry", p_parent)
	{

	}
	TextEntry(const std::string& p_name, Widget* p_parent) :
		spk::TextEntry(p_name, p_parent)
	{
		box().setSpriteSheet(TextureAtlas::instance()->spriteSheet("DefaultFrame"));
		box().setCornerSize(8);

		setPlaceholder("Enter text ...");

		label().setFont(TextureAtlas::instance()->font("DefaultFont"));
		label().setVerticalAlignment(spk::VerticalAlignment::Top);
		label().setHorizontalAlignment(spk::HorizontalAlignment::Left);

		label().setTextSize(25);
		label().setTextColor(spk::Color(255, 255, 255));

		label().setOutlineSize(3);
		label().setOutlineColor(spk::Color(0, 0, 0));
		label().setOutlineStyle(spk::Font::OutlineStyle::Standard);
	}
};

class Button : public spk::Button
{
private:

public:
	Button(Widget* p_parent) :
		Button("Anonymous Button", p_parent)
	{

	}
	Button(const std::string& p_name, Widget* p_parent) :
		spk::Button(p_name, p_parent)
	{
		box(spk::Button::State::Released).setSpriteSheet(TextureAtlas::instance()->spriteSheet("DefaultFrame"));
		box(spk::Button::State::Released).setCornerSize(8);
		
		box(spk::Button::State::Pressed).setSpriteSheet(TextureAtlas::instance()->spriteSheet("DarkFrame"));
		box(spk::Button::State::Pressed).setCornerSize(8);
	}
};

CustomTextureManager::CustomTextureManager()
{
	loadSpriteSheet("DefaultFrame", "Playground/resources/texture/buttonPressed.png", spk::Vector2UInt(3, 3));
	loadSpriteSheet("DarkFrame", "Playground/resources/texture/buttonReleased.png", spk::Vector2UInt(3, 3));
	loadSpriteSheet("TitleBackground", "Playground/resources/texture/buttonPressed.png", spk::Vector2UInt(3, 3));

	loadFont("DefaultFont", "Playground/resources/font/Roboto-Regular.ttf");
	loadFont("TitleFont", "Playground/resources/font/Heavitas.ttf");
}

CustomTextureManager::~CustomTextureManager()
{
	for (auto& [key, element] : _fonts)
	{
		delete element;
	}
}

spk::Font* CustomTextureManager::loadFont(const std::string& p_fontName, const std::filesystem::path& p_fontPath)
{
	if (_fonts.contains(p_fontName) == true)
		spk::throwException("Can't load a font named [" + p_fontName + "] inside TextureAtlas : font already loaded");
	_fonts[p_fontName] = new spk::Font(p_fontPath);
	return (_fonts[p_fontName]);
}

spk::Font* CustomTextureManager::font(const std::string& p_fontName)
{
	if (_fonts.contains(p_fontName) == false)
		spk::throwException("Can't return a font named [" + p_fontName + "] inside TextureAtlas\nNo font loaded with desired name");
	return (_fonts[p_fontName]);
}

class MainMenuPanel : public spk::Panel
{
private:
	class SelectorWidget : public spk::Widget
	{
	private:
		Frame _backgroundFrame;
		TextLabel _titleLabel;

		void _onUpdate()
		{
			
		}
	
		void _onGeometryChange()
		{
			_backgroundFrame.setGeometry(0, size());

			spk::Vector2Int titleLabelAnchor = _titleLabel.box().cornerSize();
			spk::Vector2Int titleLabelSize = spk::Vector2Int(size().x - _titleLabel.box().cornerSize().x * 2, _titleLabel.box().cornerSize().y * 5);
			
			_titleLabel.setGeometry(titleLabelAnchor, titleLabelSize);
			_titleLabel.label().setTextSize(_titleLabel.label().font()->computeOptimalTextSize(_titleLabel.label().text(), _titleLabel.label().outlineSize(), _titleLabel.label().outlineStyle(), titleLabelSize - _titleLabel.box().cornerSize() * 2));
		}
	
		void _onRender()
		{
			
		}
	
	public:
		SelectorWidget(spk::Widget* p_parent) : SelectorWidget("Anonymous SelectorWidget", p_parent){}
	
		SelectorWidget(const std::string& p_name, spk::Widget* p_parent) :
			spk::Widget(p_name, p_parent),
			_backgroundFrame("BackgroundFrame", this),
			_titleLabel("Title Background", &_backgroundFrame)
		{
			_backgroundFrame.box().setCornerSize(4);
			_backgroundFrame.activate();

			_titleLabel.label().setFont(TextureAtlas::instance()->font("TitleFont"));
			_titleLabel.label().setText("World Of Electron");
			_titleLabel.label().setVerticalAlignment(spk::VerticalAlignment::Centered);
			_titleLabel.label().setHorizontalAlignment(spk::HorizontalAlignment::Centered);
			_titleLabel.label().setOutlineStyle(spk::Font::OutlineStyle::None);
			_titleLabel.label().setOutlineSize(3);
			_titleLabel.label().setOutlineColor(spk::Color(50, 50, 50));
			_titleLabel.label().setTextColor(spk::Color(255, 15, 0));


			_titleLabel.box().setSpriteSheet(TextureAtlas::instance()->spriteSheet("TitleBackground"));
			_titleLabel.box().setCornerSize(16);
			
			_titleLabel.activate();
		}
	};
	
	SelectorWidget _selectorWidget;

	void _onUpdate()
	{
		
	}

	void _onGeometryChange()
	{
		spk::Vector2Int selectorSize = spk::Vector2Int(std::min(400.0f, ((size().x) / 3.0f)), std::min(300.0f, ((size().y) / 1.5f)));
		spk::Vector2Int selectorAnchor = (size() - selectorSize) / 2;
		_selectorWidget.setGeometry(selectorAnchor, selectorSize);
	}

	void _onRender()
	{
		
	}

public:
	MainMenuPanel(spk::Widget* p_parent) : MainMenuPanel("Anonymous MainMenuPanel", p_parent) {}
	
	MainMenuPanel(const std::string& p_name, spk::Widget* p_parent) :	
		spk::Panel(p_name, p_parent),
		_selectorWidget("Main menu Selector", this)
	{
		_selectorWidget.activate();
	}

};

class MainWidget : public spk::Widget
{
private:
	TextureAtlas::Instanciator _textureManagerInstanciator;

	MainMenuPanel _mainMenuPanel;

	void _onUpdate()
	{
		
	}

	void _onGeometryChange()
	{
		_mainMenuPanel.setGeometry(0, size());
	}

	void _onRender()
	{
		
	}

public:
	MainWidget(spk::Widget* p_parent) : MainWidget("Anonymous MainWidget", p_parent){}

	MainWidget(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_textureManagerInstanciator(),
		_mainMenuPanel("MainPanel", this)
	{
		_mainMenuPanel.activate();
	}
};

int main()
{
    spk::Application app = spk::Application("World Of Electron", spk::Vector2UInt(800, 640), spk::Application::Mode::Multithread);

	MainWidget mainWidget(nullptr);
	mainWidget.setGeometry(0, app.size());
	mainWidget.activate();

    return (app.run());
}