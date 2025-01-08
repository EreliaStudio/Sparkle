#include "texture_manager.hpp"

#include "utils/spk_string_utils.hpp"

TextureManager::TextureManager()
{
	loadSpriteSheet(L"chunkSpriteSheet", L"playground/resources/test.png", spk::Vector2UInt(2, 2));
	loadSpriteSheet(L"frameTexture", L"playground/resources/nineSlice.png", spk::Vector2UInt(3, 3));
	loadSpriteSheet(L"pushButtonNineSlice", L"playground/resources/pushButton.png", spk::Vector2UInt(3, 3));
	loadFont(L"defaultFont", "playground/resources/arial.ttf");
}

spk::SafePointer<spk::Image> TextureManager::loadImage(const std::wstring& p_name, const std::filesystem::path& p_imagePath)
{
	if (_textures.contains(p_name) == true)
	{
		throw std::runtime_error("Texture already loaded");
	}

	_textures[p_name] = std::make_unique<spk::Image>(p_imagePath);
	_fonts[p_name] = nullptr;

	return (image(p_name));
}

spk::SafePointer<spk::SpriteSheet> TextureManager::loadSpriteSheet(const std::wstring& p_name, const std::filesystem::path& p_spriteSheetPath, const spk::Vector2UInt& p_spriteSheetSize)
{
	if (_textures.contains(p_name) == true)
	{
		throw std::runtime_error("Texture already loaded");
	}

	_textures[p_name] = std::make_unique<spk::SpriteSheet>(p_spriteSheetPath, p_spriteSheetSize);
	_fonts[p_name] = nullptr;

	return (spriteSheet(p_name));
}

spk::SafePointer<spk::Font> TextureManager::loadFont(const std::wstring& p_name, const std::filesystem::path& p_fontPath)
{
	if (_fonts.contains(p_name) == true)
	{
		throw std::runtime_error("Texture already loaded");
	}

	_fonts[p_name] = std::make_unique<spk::Font>(p_fontPath);
	_textures[p_name] = nullptr;

	return (font(p_name));
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

spk::SafePointer<spk::Font> TextureManager::font(const std::wstring& p_name)
{
	if (_fonts.contains(p_name) == false)
	{
		throw std::runtime_error("Texture [" + spk::StringUtils::wstringToString(p_name) + "] not found");
	}

	spk::Font* convertedPtr = dynamic_cast<spk::Font*>(_fonts[p_name].get());

	if (convertedPtr == nullptr)
	{
		throw std::runtime_error("Texture [" + spk::StringUtils::wstringToString(p_name) + "] is not a Font");
	}

	return (spk::SafePointer<spk::Font>(convertedPtr));
}