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

	Vector2Int Font::Atlas::computeCharSize(const wchar_t& p_char) const
	{
		const GlyphData& glyphData = this->operator[](p_char);
		return Vector2Int(glyphData.size.x, glyphData.size.y);
	}

	Vector2Int Font::Atlas::computeStringSize(const std::string& p_string) const
	{
		int totalWidth = 0;
		int maxHeight = 0;
		int minHeight = 0;

		for (char ch : p_string)
		{
			const Atlas::GlyphData& glyphData = this->operator[](ch);
			totalWidth += glyphData.step.x;
			
			maxHeight = std::max(maxHeight, glyphData.position[4].y);
			minHeight = std::min(minHeight, glyphData.position[0].y);
		}

		int totalHeight = maxHeight - minHeight;
		return Vector2Int(totalWidth, totalHeight);
	}

	Vector2Int Font::computeCharSize(const wchar_t& p_char, size_t p_size, size_t p_outlineSize, const spk::Font::OutlineStyle& p_outlineStyle)
	{
		return (atlas(p_size, p_outlineSize, p_outlineStyle).computeCharSize(p_char));
	}

	Vector2Int Font::computeStringSize(const std::string& p_string, size_t p_size, size_t p_outlineSize, const spk::Font::OutlineStyle& p_outlineStyle)
	{
		return (atlas(p_size, p_outlineSize, p_outlineStyle).computeStringSize(p_string));
	}

	size_t Font::computeOptimalTextSize(const std::string& p_string, size_t p_outlineSize, const spk::Font::OutlineStyle& p_outlineStyle, const Vector2Int& p_textArea)
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
				Vector2Int tmp_size = computeStringSize(p_string, result + deltas[i], p_outlineSize, p_outlineStyle);
				if (tmp_size.x >= p_textArea.x || tmp_size.y >= p_textArea.y)
				{
					enough = true;
				}
				else
				{
					result += deltas[i];
				}
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

std::ostream& operator << (std::ostream& p_os, const spk::Font::OutlineStyle& p_outlineStyle)
{
	switch (p_outlineStyle)
	{
		case spk::Font::OutlineStyle::Manhattan:
			p_os << "Manhattan"; return (p_os);
		case spk::Font::OutlineStyle::Pixelized:
			p_os << "Pixelized"; return (p_os);
		case spk::Font::OutlineStyle::SharpEdge:
			p_os << "SharpEdge"; return (p_os);
		case spk::Font::OutlineStyle::Standard:
			p_os << "Standard"; return (p_os);
		case spk::Font::OutlineStyle::None:
			p_os << "None"; return (p_os);
		default:
			p_os << "Unknown outline style"; return (p_os);
	}
}