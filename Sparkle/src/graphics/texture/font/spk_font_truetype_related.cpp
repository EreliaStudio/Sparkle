#include "graphics/texture/font/spk_font.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include <fstream>

#define STB_TRUETYPE_IMPLEMENTATION
#include "external_libraries/stb_truetype.h"

namespace spk
{
	std::vector<std::pair<int, int>> UnicodeBlocks = {
	{0x0000, 0x007F}, // Basic Latin
	{0x0080, 0x00FF}, // Latin-1 Supplement
	// Add other ranges as needed
};

void Font::Atlas::loadAllRenderableGlyphs()
{
	std::unordered_set<int> renderableGlyphs;

	for (const auto& block : UnicodeBlocks)
	{
		for (int codepoint = block.first; codepoint <= block.second; ++codepoint)
		{
			if (renderableGlyphs.find(codepoint) == renderableGlyphs.end())
			{
				int glyphIndex = stbtt_FindGlyphIndex(&_fontInfo, codepoint);
				if (glyphIndex != 0)
				{
					std::wcout << "Setting up char [" << codepoint << "]" << std::endl;
					operator[](static_cast<wchar_t>(codepoint));
					renderableGlyphs.insert(codepoint);
				}
			}
		}
	}
}

	spk::Vector2UInt Font::Atlas::_computeGlyphPosition(const spk::Vector2UInt& p_glyphSize)
	{
		if ((_nextGlyphAnchor.x + p_glyphSize.x) >= (_quadrantAnchor.x + _quadrantSize.x))
		{
			_nextGlyphAnchor = _nextLineAnchor;
		}

		spk::Vector2UInt result = _nextGlyphAnchor;

		_nextGlyphAnchor.x += p_glyphSize.x;

		if (_nextLineAnchor.y < result.y + p_glyphSize.y)
		{
			_nextLineAnchor.y = result.y + p_glyphSize.y;
		}

		switch (_currentQuadrant)
		{
			case Quadrant::TopLeft:
			{				
				if (_nextLineAnchor.y >= _quadrantAnchor.y + _quadrantSize.y)
				{
					_currentQuadrant = Quadrant::TopRight;
					_resizeData(_size * 2);
					_quadrantAnchor = spk::Vector2UInt(_size.x / 2, 0);
					result = _nextGlyphAnchor = _nextLineAnchor = _quadrantAnchor;
					_nextGlyphAnchor.x += p_glyphSize.x;
					if (_nextLineAnchor.y < result.y + p_glyphSize.y)
					{
						_nextLineAnchor.y = result.y + p_glyphSize.y;
					}
				}

				break;
			}
			case Quadrant::TopRight:
			{
				if (_nextLineAnchor.y >= _quadrantAnchor.y + _quadrantSize.y)
				{
					_currentQuadrant = Quadrant::DownLeft;
					_quadrantAnchor = spk::Vector2UInt(0, _size.y / 2);
					result = _nextGlyphAnchor = _nextLineAnchor = _quadrantAnchor;
					_nextGlyphAnchor.x += p_glyphSize.x;
					if (_nextLineAnchor.y < result.y + p_glyphSize.y)
					{
						_nextLineAnchor.y = result.y + p_glyphSize.y;
					}
				}
				break;
			}
			case Quadrant::DownLeft:
			{
				if (_nextLineAnchor.y >= _quadrantAnchor.y + _quadrantSize.y)
				{
					_currentQuadrant = Quadrant::DownRight;
					_quadrantAnchor = spk::Vector2UInt(_size.x / 2, _size.y / 2);
					result = _nextGlyphAnchor = _nextLineAnchor = _quadrantAnchor;
					_nextGlyphAnchor.x += p_glyphSize.x;
					if (_nextLineAnchor.y < result.y + p_glyphSize.y)
					{
						_nextLineAnchor.y = result.y + p_glyphSize.y;
					}
				}
				break;
			}
			case Quadrant::DownRight:
			{	
				if (_nextLineAnchor.y >= _quadrantAnchor.y + _quadrantSize.y)
				{
					_currentQuadrant = Quadrant::TopRight;
					_resizeData(_size * 2);
					_quadrantAnchor = spk::Vector2UInt(_size.x / 2, 0);
					result = _nextGlyphAnchor = _nextLineAnchor = _quadrantAnchor;
					_nextGlyphAnchor.x += p_glyphSize.x;
					if (_nextLineAnchor.y < result.y + p_glyphSize.y)
					{
						_nextLineAnchor.y = result.y + p_glyphSize.y;
					}
				}

				break;
			}
		}

		return (result);
	}


	void Font::Atlas::_loadGlyph(const wchar_t& p_char)
	{
		Glyph glyph;

		float scale = stbtt_ScaleForMappingEmToPixels(&_fontInfo, static_cast<float>(_textSize));

		int width, height, xOffset, yOffset;
		uint8_t* glyphBitmap = stbtt_GetCodepointSDF(&_fontInfo, scale, p_char, static_cast<int>(_outlineSize), 255, 256.0f / static_cast<float>(_outlineSize), &width, &height, &xOffset, &yOffset);

		if (glyphBitmap == nullptr)
		{
			_glyphs[p_char] = _unknownGlyph;
			return ;
		}

		int advance;
		stbtt_GetCodepointHMetrics(&_fontInfo, p_char, &advance, NULL);

		glyph.size = spk::Vector2UInt(width, height);

		spk::Vector2Int glyphPosition = _computeGlyphPosition(glyph.size);

		_applyGlyphPixel(glyphBitmap, glyphPosition, glyph.size);

		glyph.positions[0] = spk::Vector2Int(0, yOffset);
		glyph.positions[1] = spk::Vector2Int(0, yOffset + height);
		glyph.positions[2] = spk::Vector2Int(width, yOffset);
		glyph.positions[3] = spk::Vector2Int(width, yOffset + height);

		glyph.UVs[0] = spk::Vector2(static_cast<float>(glyphPosition.x) / _size.x, static_cast<float>(glyphPosition.y) / _size.y);
		glyph.UVs[1] = spk::Vector2(static_cast<float>(glyphPosition.x) / _size.x, static_cast<float>(glyphPosition.y + glyph.size.y) / _size.y);
		glyph.UVs[2] = spk::Vector2(static_cast<float>(glyphPosition.x + glyph.size.x) / _size.x, static_cast<float>(glyphPosition.y) / _size.y);
		glyph.UVs[3] = spk::Vector2(static_cast<float>(glyphPosition.x + glyph.size.x) / _size.x, static_cast<float>(glyphPosition.y + glyph.size.y) / _size.y);

		glyph.step = spk::Vector2(std::ceil(advance * scale) + _outlineSize * 2, 0);
		glyph.size = glyph.positions[3] - glyph.positions[0];

		_glyphs[p_char] = glyph;

		stbtt_FreeBitmap(glyphBitmap, nullptr);

		_needUpload = true;
		spk::Pipeline::Texture::resetLastActiveTexture();
	}
	
	void Font::_loadFileData(const std::filesystem::path& p_path)
	{
		_fontData = readFileContentAsBytes(p_path);
		stbtt_InitFont(&_fontInfo, _fontData.data(), 0);
	}
}