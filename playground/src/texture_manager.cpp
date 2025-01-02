#include "texture_manager.hpp"

#include "utils/spk_string_utils.hpp"

TextureManager::TextureManager()
{
	loadSpriteSheet(L"chunkSpriteSheet", new spk::SpriteSheet(L"playground/resources/test.png", spk::Vector2UInt(2, 2)));
}

spk::SafePointer<spk::Image> TextureManager::loadImage(const std::wstring& p_name, spk::Image* p_image)
{
	if (_textures.contains(p_name) == true)
	{
		throw std::runtime_error("Texture already loaded");
	}
	_textures[p_name] = std::unique_ptr<spk::Image>(p_image);
	spk::Image* convertedPtr = dynamic_cast<spk::Image*>(_textures[p_name].get());
	return (spk::SafePointer<spk::Image>(convertedPtr));
}

spk::SafePointer<spk::SpriteSheet> TextureManager::loadSpriteSheet(const std::wstring& p_name, spk::SpriteSheet* p_spriteSheet)
{
	if (_textures.contains(p_name) == true)
	{
		throw std::runtime_error("Texture already loaded");
	}
	_textures[p_name] = std::unique_ptr<spk::SpriteSheet>(p_spriteSheet);
	spk::SpriteSheet* convertedPtr = dynamic_cast<spk::SpriteSheet*>(_textures[p_name].get());
	return (spk::SafePointer<spk::SpriteSheet>(convertedPtr));
}

bool TextureManager::contain(const std::wstring& p_name)
{
	return (_textures.contains(p_name));
}

spk::SafePointer<spk::Image> TextureManager::image(const std::wstring& p_name)
{
	if (!_textures.contains(p_name))
	{
		throw std::runtime_error("Texture [" + spk::StringUtils::wstringToString(p_name) + "] not found");
	}

	spk::Image* convertedPtr = dynamic_cast<spk::Image*>(_textures[p_name].get());

	if (convertedPtr == nullptr)
	{
		throw std::runtime_error("Texture [" + spk::StringUtils::wstringToString(p_name) + "] is not a Image");
	}
	return (convertedPtr);
}

spk::SafePointer<spk::SpriteSheet> TextureManager::spriteSheet(const std::wstring& p_name)
{
	if (!_textures.contains(p_name))
	{
		throw std::runtime_error("Texture [" + spk::StringUtils::wstringToString(p_name) + "] not found");
	}

	spk::SpriteSheet* convertedPtr = dynamic_cast<spk::SpriteSheet*>(_textures[p_name].get());

	if (convertedPtr == nullptr)
	{
		throw std::runtime_error("Texture [" + spk::StringUtils::wstringToString(p_name) + "] is not a SpriteSheet");
	}

	return (spk::SafePointer<spk::SpriteSheet>(convertedPtr));
}