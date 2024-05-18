#include "graphics/spk_texture_atlas.hpp"

namespace spk
{
	TextureAtlas::TextureAtlas()
	{

	}

	TextureAtlas::~TextureAtlas()
	{
		for (auto& [key, element] : _loadedTextures)
		{
			delete element;
		}
	}

	Texture* TextureAtlas::loadTexture(const std::string& p_textureName, const uint8_t* p_textureData, const Vector2UInt& p_textureSize,
			const Texture::Format& p_format, const Texture::Filtering& p_filtering,
			const Texture::Wrap& p_wrap, const Texture::Mipmap& p_mipmap)
	{
		if (_loadedTextures.contains(p_textureName) == true)
			throwException("Texture [" + p_textureName + "] is already loaded");

		Texture* result = new Texture(p_textureData, p_textureSize, p_format, p_filtering, p_wrap, p_mipmap);
		_loadedTextures[p_textureName] = result;
		return (result);
	}
	
	Image* TextureAtlas::loadImage(const std::string& p_textureName, const std::filesystem::path& p_path)
	{
		if (_loadedTextures.contains(p_textureName) == true)
			throwException("Texture [" + p_textureName + "] is already loaded");

		Image* result = new Image(p_path);
		_loadedTextures[p_textureName] = result;
		return (result);
	}
	
	SpriteSheet* TextureAtlas::loadSpriteSheet(const std::string& p_textureName, const std::filesystem::path& p_path, const Vector2UInt& p_spriteSheetSize)
	{
		if (_loadedTextures.contains(p_textureName) == true)
			throwException("Texture [" + p_textureName + "] is already loaded");

		SpriteSheet* result = new SpriteSheet(p_path, p_spriteSheetSize);
		_loadedTextures[p_textureName] = result;
		return (result);
	}

	Texture* TextureAtlas::texture(const std::string& p_textureName) const
	{
		if (_loadedTextures.contains(p_textureName) == false)
			throwException("Can't return a Texture named [" + p_textureName + "] inside TextureAtlas\nNo such Texture loaded with desired name");
		return (_loadedTextures.at(p_textureName));
	}

	Image* TextureAtlas::image(const std::string& p_textureName) const
	{
		if (_loadedTextures.contains(p_textureName) == false)
			throwException("Can't return a Image named [" + p_textureName + "] inside TextureAtlas\nNo such Image loaded with desired name");
		return (dynamic_cast<Image*>(_loadedTextures.at(p_textureName)));
	}
	
	SpriteSheet* TextureAtlas::spriteSheet(const std::string& p_textureName) const
	{
		if (_loadedTextures.contains(p_textureName) == false)
			throwException("Can't return a SpriteSheet named [" + p_textureName + "] inside TextureAtlas\nNo such SpriteSheet loaded with desired name");
		return (dynamic_cast<SpriteSheet*>(_loadedTextures.at(p_textureName)));
	}
}