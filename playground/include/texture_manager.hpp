#pragma once

#include <unordered_map>

#include "structure/design_pattern/spk_singleton.hpp"

#include "structure/graphics/opengl/spk_texture_object.hpp"

#include "structure/graphics/texture/spk_image.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"

class TextureManager : public spk::Singleton<TextureManager>
{
	friend class spk::Singleton<TextureManager>;
private:
	std::unordered_map<std::wstring, std::unique_ptr<spk::OpenGL::TextureObject>> _textures;

	TextureManager();

public:
	spk::SafePointer<spk::Image> loadImage(const std::wstring& p_name, spk::Image* p_image);
	spk::SafePointer<spk::SpriteSheet> loadSpriteSheet(const std::wstring& p_name, spk::SpriteSheet* p_spriteSheet);

	bool contain(const std::wstring& p_name);
	
	spk::SafePointer<spk::Image> image(const std::wstring& p_name);
	spk::SafePointer<spk::SpriteSheet> spriteSheet(const std::wstring& p_name);
};