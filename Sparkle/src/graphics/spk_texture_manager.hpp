#pragma once

#include <unordered_map>
#include <string>

#include "design_pattern/spk_singleton.hpp"
#include "graphics/texture/spk_image.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"


namespace spk
{
	class TextureManager : public Singleton<TextureManager>
	{
	friend class Singleton<TextureManager>;

	private:
		std::unordered_map<std::string, Texture*> _loadedTexture;

	protected:
		TextureManager();

	public:
		Texture*  loadTexture(const std::string& p_textureName,
				const uint8_t* p_textureData, const Vector2UInt& p_textureSize,
				const Texture::Format& p_format, const Texture::Filtering& p_filtering,
				const Texture::Wrap& p_wrap, const Texture::Mipmap& p_mipmap);
		Image* loadImage(const std::string& p_textureName,
				const std::filesystem::path& p_path);
		SpriteSheet* loadSpriteSheet(const std::string& p_textureName,
				const std::filesystem::path& p_path, const Vector2UInt& p_spriteSheetSize);

		Texture* texture(const std::string& p_textureName) const;
		Image* image(const std::string& p_textureName) const;
		SpriteSheet* spriteSheet(const std::string& p_textureName) const;
	};	
}