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

	Vector2Int Font::computeCharSize(const wchar_t& p_char, size_t p_size, size_t p_outlineSize)
	{
		const Atlas& atlasRef = atlas(p_size, p_outlineSize, OutlineStyle::Standard);
		const Atlas::GlyphData& glyphData = atlasRef[p_char];
		return Vector2Int(glyphData.size.x, glyphData.size.y);
	}

	Vector2Int Font::computeStringSize(const std::string& p_string, size_t p_size, size_t p_outlineSize)
	{
		int totalWidth = 0;
		int maxHeight = 0;
		int minHeight = 0;

		for (char ch : p_string)
		{
			Vector2Int charSize = computeCharSize(ch, p_size, p_outlineSize);
			totalWidth += charSize.x;
			
			maxHeight = std::max(maxHeight, charSize.y);
			minHeight = std::min(minHeight, 0);
		}

		int totalHeight = maxHeight - minHeight;
		return Vector2Int(totalWidth, totalHeight);
	}

	size_t Font::computeOptimalTextSize(const std::string& p_string, size_t p_outlineSize, Vector2Int p_textArea)
	{
		std::vector<int> deltas = { 100, 50, 20, 10, 1 };
		size_t result = 2;

		if (p_string == "")
			return (p_textArea.y);

		for (int i = 0; i < deltas.size(); i++)
		{
			bool enough = false;
			while (enough == false)
			{
				Vector2Int tmp_size = computeStringSize(p_string, result + deltas[i], p_outlineSize);
				if (tmp_size.x >= p_textArea.x || tmp_size.y >= p_textArea.y)
					enough = true;
				else
					result += deltas[i];
			}
		}
		return (result);
	}

	const Font::Atlas& Font::atlas(const size_t &p_fontSize, const size_t &p_outlineSize, const OutlineStyle& p_outlineStype) const
	{
		Key tmpKey = Key(p_fontSize, p_outlineSize, p_outlineStype);
		if (_fontAtlas.contains(tmpKey) == false)
		{
			_fontAtlas.emplace(tmpKey, std::move(Atlas(_fontData, _fontConfiguration, tmpKey)));
		}
		return _fontAtlas.at(tmpKey);
	}

	Font::Atlas& Font::atlas(const size_t &p_fontSize, const size_t &p_outlineSize, const OutlineStyle& p_outlineStype)
	{
		Key tmpKey = Key(p_fontSize, p_outlineSize, p_outlineStype);
		if (_fontAtlas.contains(tmpKey) == false)
		{
			_fontAtlas.emplace(tmpKey, std::move(Atlas(_fontData, _fontConfiguration, tmpKey)));
		}
		return _fontAtlas[tmpKey];
	}
}