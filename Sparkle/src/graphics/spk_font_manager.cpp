#include "graphics/spk_font_manager.hpp"

namespace spk
{
	FontManager::FontManager()
    {

    }

    FontManager::~FontManager()
    {
        for (auto& [key, element] : _loadedFont)
        {
            delete element;
        }
    }

    Font* FontManager::loadFont(const std::string& p_textureName, const std::filesystem::path& p_path)
    {
        if (_loadedFont.contains(p_textureName) == true)
            throwException("Texture [" + p_textureName + "] is already loaded");

		Font* result = new Font(p_path);
        _loadedFont[p_textureName] = result;
		return (result);
    }

    Font* FontManager::font(const std::string& p_textureName) const
    {
        if (_loadedFont.contains(p_textureName) == false)
            return (nullptr);
        return (dynamic_cast<Font*>(_loadedFont.at(p_textureName)));
    }
}