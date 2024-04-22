#include "graphics/spk_texture_manager.hpp"

namespace spk
{
	TextureManager::TextureManager()
    {

    }

    TextureManager::~TextureManager()
    {
        for (auto& [key, element] : _loadedTextures)
        {
            delete element;
        }
    }

    Texture* TextureManager::loadTexture(const std::string& p_textureName, const uint8_t* p_textureData, const Vector2UInt& p_textureSize,
			const Texture::Format& p_format, const Texture::Filtering& p_filtering,
			const Texture::Wrap& p_wrap, const Texture::Mipmap& p_mipmap)
    {
        if (_loadedTextures.contains(p_textureName) == true)
            throwException("Texture [" + p_textureName + "] is already loaded");

		Texture* result = new Texture(p_textureData, p_textureSize, p_format, p_filtering, p_wrap, p_mipmap);
        _loadedTextures[p_textureName] = result;
		return (result);
    }
    
    Image* TextureManager::loadImage(const std::string& p_textureName, const std::filesystem::path& p_path)
    {
        if (_loadedTextures.contains(p_textureName) == true)
            throwException("Texture [" + p_textureName + "] is already loaded");

		Image* result = new Image(p_path);
        _loadedTextures[p_textureName] = result;
		return (result);
    }
    
    SpriteSheet* TextureManager::loadSpriteSheet(const std::string& p_textureName, const std::filesystem::path& p_path, const Vector2UInt& p_spriteSheetSize)
    {
        if (_loadedTextures.contains(p_textureName) == true)
            throwException("Texture [" + p_textureName + "] is already loaded");

		SpriteSheet* result = new SpriteSheet(p_path, p_spriteSheetSize);
        _loadedTextures[p_textureName] = result;
		return (result);
    }

    Texture* TextureManager::texture(const std::string& p_textureName) const
    {
        if (_loadedTextures.contains(p_textureName) == false)
            return (nullptr);
        return (_loadedTextures.at(p_textureName));
    }

    Image* TextureManager::image(const std::string& p_textureName) const
    {
        if (_loadedTextures.contains(p_textureName) == false)
            return (nullptr);
        return (dynamic_cast<Image*>(_loadedTextures.at(p_textureName)));
    }
    
    SpriteSheet* TextureManager::spriteSheet(const std::string& p_textureName) const
    {
        if (_loadedTextures.contains(p_textureName) == false)
            return (nullptr);
        return (dynamic_cast<SpriteSheet*>(_loadedTextures.at(p_textureName)));
    }
}