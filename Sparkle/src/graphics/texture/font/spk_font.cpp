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

	const Font::Atlas& Font::atlas(const size_t &p_fontSize, const size_t &p_outlineSize, const OutlineStyle& p_outlineStype) const
	{
		Key tmpKey = Key(p_fontSize, p_outlineSize, p_outlineStype);
		if (_fontAtlas.contains(tmpKey) == false)
		{
			_fontAtlas.emplace(tmpKey, std::move(Atlas(_fontData, _fontConfiguration, tmpKey)));
		}
		return _fontAtlas[tmpKey];
	}
}