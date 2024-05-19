#include "graphics/texture/font/spk_font.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"
#include <fstream>

#define STB_TRUETYPE_IMPLEMENTATION
#include "external_libraries/stb_truetype.h"

namespace spk
{
	void Font::Atlas::loadAllRenderableGlyphs()
	{
		for (int codepoint = 0; codepoint <= 0x10FFFF; ++codepoint)
		{
			if (stbtt_FindGlyphIndex(&_fontInfo, codepoint) != 0)
			{
				operator[](static_cast<wchar_t>(codepoint));
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


	void Font::Atlas::_loadGlyph(const wchar_t& p_glyph)
	{
		Glyph glyph;

		float scale = stbtt_ScaleForMappingEmToPixels(&_fontInfo, static_cast<float>(_textSize));

		int width, height, xOffset, yOffset;
		uint8_t* glyphBitmap = stbtt_GetCodepointSDF(&_fontInfo, scale, p_glyph, static_cast<int>(_outlineSize * 2 + 1), 255, 255.0f / static_cast<float>(_outlineSize * 2), &width, &height, &xOffset, &yOffset);

		if (glyphBitmap == nullptr)
		{
			_glyphs[p_glyph] = _unknownGlyph;
			return ;
		}

		int advance;
		stbtt_GetCodepointHMetrics(&_fontInfo, p_glyph, &advance, NULL);

		spk::Vector2UInt glyphSize = spk::Vector2UInt(width, height);

		spk::Vector2UInt glyphPosition = _computeGlyphPosition(glyphSize);

		_applyGlyphPixel(glyphBitmap, glyphPosition, glyphSize);

		glyph.positions[0] = glyphPosition;
		glyph.positions[1] = spk::Vector2UInt(glyphPosition.x + glyphSize.x, glyphPosition.y);
		glyph.positions[2] = spk::Vector2UInt(glyphPosition.x + glyphSize.x, glyphPosition.y + glyphSize.y);
		glyph.positions[3] = spk::Vector2UInt(glyphPosition.x, glyphPosition.y + glyphSize.y);

		glyph.UVs[0] = spk::Vector2(static_cast<float>(glyphPosition.x) / _size.x, static_cast<float>(glyphPosition.y) / _size.y);
		glyph.UVs[1] = spk::Vector2(static_cast<float>(glyphPosition.x + glyphSize.x) / _size.x, static_cast<float>(glyphPosition.y) / _size.y);
		glyph.UVs[2] = spk::Vector2(static_cast<float>(glyphPosition.x + glyphSize.x) / _size.x, static_cast<float>(glyphPosition.y + glyphSize.y) / _size.y);
		glyph.UVs[3] = spk::Vector2(static_cast<float>(glyphPosition.x) / _size.x, static_cast<float>(glyphPosition.y + glyphSize.y) / _size.y);

		glyph.step = spk::Vector2(advance * scale, 0);

		_glyphs[p_glyph] = glyph;

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