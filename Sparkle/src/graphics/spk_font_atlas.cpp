#include "graphics/spk_font_atlas.hpp"

namespace spk
{
	FontAtlas::FontAtlas()
	{

	}

	FontAtlas::FontAtlas(const spk::JSON::Object& p_object)
	{
		if (p_object.isArray() == true)
		{
			for (size_t i = 0; i < p_object.size(); i++)
			{
				loadFont(p_object[i]["Name"].as<std::string>(), p_object[i]["Path"].as<std::string>());
			}
		}
		else
		{
			spk::throwException("FontAtlas can't be instanciated with the wrong format");
		}
	}

	FontAtlas::~FontAtlas()
	{
		for (auto& [key, element] : _loadedFonts)
		{
			delete element;
		}
	}

	Font* FontAtlas::loadFont(const std::string& p_textureName, const std::filesystem::path& p_path)
	{
		if (_loadedFonts.contains(p_textureName) == true)
			throwException("Font [" + p_textureName + "] is already loaded");

		Font* result = new Font(p_path);
		_loadedFonts[p_textureName] = result;
		return (result);
	}

	Font* FontAtlas::font(const std::string& p_textureName) const
	{
		if (_loadedFonts.contains(p_textureName) == false)
			return (nullptr);
		return (dynamic_cast<Font*>(_loadedFonts.at(p_textureName)));
	}
}