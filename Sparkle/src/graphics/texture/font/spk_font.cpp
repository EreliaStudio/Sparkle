#include "graphics/texture/font/spk_font.hpp"
#include <fstream>

namespace spk
{
	Font::Font(const std::filesystem::path &p_path)
	{
		std::ifstream file(p_path, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			throwException("Failed to open font file [" + p_path.string() + "].");
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		_fontData.resize(static_cast<size_t>(size));
		if (!file.read(reinterpret_cast<char *>(_fontData.data()), size))
		{
			throwException("Failed to read font file [" + p_path.string() + "].");
		}

		_fontConfiguration = Configuration(p_path.string(), _fontData);
	}

	Font::Atlas* Font::atlas(const size_t &p_fontSize, const size_t &p_outlineSize) const
	{
		return (this->operator[](Key(p_fontSize, p_outlineSize)));
	}

	Font::Atlas* Font::atlas(const size_t &p_fontSize, const size_t &p_outlineSize)
	{
		return (this->operator[](Key(p_fontSize, p_outlineSize)));
	}

	Font::Atlas* Font::atlas(const Font::Key &p_fontAtlasKey) const
	{
		return (this->operator[](p_fontAtlasKey));
	}

	Font::Atlas* Font::atlas(const Font::Key &p_fontAtlasKey)
	{
		return (this->operator[](p_fontAtlasKey));
	}

	Font::Atlas* Font::operator[](const Font::Key &p_fontAtlasKey) const
	{
		if (_fontAtlas.contains(p_fontAtlasKey) == false)
			throwException("Atlas cannot be instancied inside a const Font object");
		return _fontAtlas.at(p_fontAtlasKey);
	}

	Font::Atlas* Font::operator[](const Font::Key &p_fontAtlasKey)
	{

		if (_fontAtlas.contains(p_fontAtlasKey) == false)
		{
			_fontAtlas.emplace(p_fontAtlasKey, new Atlas(_fontData, _fontConfiguration, p_fontAtlasKey));
		}
		return _fontAtlas[p_fontAtlasKey];
	}
}