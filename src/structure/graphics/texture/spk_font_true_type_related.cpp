#include "structure/graphics/texture/spk_font.hpp"
#include "structure/graphics/spk_pipeline.hpp"
#include <fstream>
#include <unordered_set>

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

					if (glyphIndex != 0 && _glyphs.contains(codepoint) == false)
					{
						_loadGlyph(codepoint);
						renderableGlyphs.insert(codepoint);
					}
				}
			}
		}

		_uploadTexture();
	}

	Vector2Int Font::Atlas::_computeGlyphPosition(const Vector2UInt& p_glyphSize)
	{
		if ((_nextGlyphAnchor.x + p_glyphSize.x) >= (_quadrantAnchor.x + _quadrantSize.x))
		{
			_nextGlyphAnchor = _nextLineAnchor;
		}

		Vector2Int result = _nextGlyphAnchor;

		_nextGlyphAnchor.x += p_glyphSize.x;

		if (_nextLineAnchor.y < result.y + static_cast<int>(p_glyphSize.y))
		{
			_nextLineAnchor.y = result.y + p_glyphSize.y;
		}

		switch (_currentQuadrant)
		{
		case Quadrant::TopLeft:
		{
			if (_nextLineAnchor.y >= _quadrantAnchor.y + static_cast<int>(_quadrantSize.y))
			{
				_currentQuadrant = Quadrant::TopRight;
				_resizeData(_size * 2);
				_quadrantAnchor = Vector2UInt(_size.x / 2, 0);
				result = _nextGlyphAnchor = _nextLineAnchor = _quadrantAnchor;
				_nextGlyphAnchor.x += p_glyphSize.x;
				if (_nextLineAnchor.y < result.y + static_cast<int>(p_glyphSize.y))
				{
					_nextLineAnchor.y = result.y + p_glyphSize.y;
				}
			}

			break;
		}
		case Quadrant::TopRight:
		{
			if (_nextLineAnchor.y >= _quadrantAnchor.y + static_cast<int>(_quadrantSize.y))
			{
				_currentQuadrant = Quadrant::DownLeft;
				_quadrantAnchor = Vector2UInt(0, _size.y / 2);
				result = _nextGlyphAnchor = _nextLineAnchor = _quadrantAnchor;
				_nextGlyphAnchor.x += p_glyphSize.x;
				if (_nextLineAnchor.y < result.y + static_cast<int>(p_glyphSize.y))
				{
					_nextLineAnchor.y = result.y + p_glyphSize.y;
				}
			}
			break;
		}
		case Quadrant::DownLeft:
		{
			if (_nextLineAnchor.y >= _quadrantAnchor.y + static_cast<int>(_quadrantSize.y))
			{
				_currentQuadrant = Quadrant::DownRight;
				_quadrantAnchor = Vector2UInt(_size.x / 2, _size.y / 2);
				result = _nextGlyphAnchor = _nextLineAnchor = _quadrantAnchor;
				_nextGlyphAnchor.x += p_glyphSize.x;
				if (_nextLineAnchor.y < result.y + static_cast<int>(p_glyphSize.y))
				{
					_nextLineAnchor.y = result.y + p_glyphSize.y;
				}
			}
			break;
		}
		case Quadrant::DownRight:
		{
			if (_nextLineAnchor.y >= _quadrantAnchor.y + static_cast<int>(_quadrantSize.y))
			{
				_currentQuadrant = Quadrant::TopRight;
				_resizeData(_size * 2);
				_quadrantAnchor = Vector2UInt(_size.x / 2, 0);
				result = _nextGlyphAnchor = _nextLineAnchor = _quadrantAnchor;
				_nextGlyphAnchor.x += p_glyphSize.x;
				if (_nextLineAnchor.y < result.y + static_cast<int>(p_glyphSize.y))
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
		uint8_t* glyphBitmap = stbtt_GetCodepointSDF(&_fontInfo, scale, p_char, static_cast<int>(_outlineSize) + 3, 255, 256.0f / static_cast<float>(_outlineSize), &width, &height, &xOffset, &yOffset);

		if (glyphBitmap == nullptr)
		{
			_glyphs[p_char] = _unknownGlyph;
			return;
		}

		int advance;
		stbtt_GetCodepointHMetrics(&_fontInfo, p_char, &advance, NULL);

		glyph.size = Vector2UInt(width, height);

		Vector2Int glyphPosition = _computeGlyphPosition(glyph.size);

		_applyGlyphPixel(glyphBitmap, glyphPosition, glyph.size);

		glyph.positions[0] = Vector2Int(0, yOffset);
		glyph.positions[1] = Vector2Int(0, yOffset + height);
		glyph.positions[2] = Vector2Int(width, yOffset);
		glyph.positions[3] = Vector2Int(width, yOffset + height);

		glyph.UVs[0] = Vector2(static_cast<float>(glyphPosition.x) / _size.x, static_cast<float>(glyphPosition.y) / _size.y);
		glyph.UVs[1] = Vector2(static_cast<float>(glyphPosition.x) / _size.x, static_cast<float>(glyphPosition.y + glyph.size.y) / _size.y);
		glyph.UVs[2] = Vector2(static_cast<float>(glyphPosition.x + glyph.size.x) / _size.x, static_cast<float>(glyphPosition.y) / _size.y);
		glyph.UVs[3] = Vector2(static_cast<float>(glyphPosition.x + glyph.size.x) / _size.x, static_cast<float>(glyphPosition.y + glyph.size.y) / _size.y);

		glyph.step = Vector2(std::ceil(advance * scale) + _outlineSize, 0);
		glyph.size = glyph.positions[3] - glyph.positions[0];

		_glyphs[p_char] = glyph;

		stbtt_FreeBitmap(glyphBitmap, nullptr);
	}

	void Font::_loadFileData(const std::filesystem::path& p_path)
	{
		_fontData = FileUtils::readFileAsBytes(p_path);

		stbtt_InitFont(&_fontInfo, reinterpret_cast<const unsigned char*>(_fontData.data()), 0);
	}
}