#include "graphics/texture/font/spk_font.hpp"
#include "graphics/pipeline/spk_pipeline.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external_libraries/stb_truetype.h"

namespace spk
{
	spk::Vector2UInt Font::_computeGlyphPosition(const spk::Vector2UInt& p_glyphSize)
	{
		// std::cout << "Next glyph anchor : " << _nextGlyphAnchor << std::endl;
		// std::cout << "Next line anchor : " << _nextLineAnchor << std::endl;
		// std::cout << "Texture size : " << _size << std::endl;
		// std::cout << "Glyph size : " << p_glyphSize << std::endl;
		
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
					_resizeData(_size * 2);
				}

				break;
			}
		}

		return (result);
	}

	void Font::_loadGlyph(const wchar_t& p_glyph)
	{
		Glyph glyph;

		size_t sizeInPixel = 50;

		float scale = stbtt_ScaleForMappingEmToPixels(&_fontInfo, static_cast<float>(sizeInPixel));

		int padding = static_cast<int>(sizeInPixel) / 2;
		int width, height, xOffset, yOffset;
		uint8_t* glyphBitmap = stbtt_GetCodepointSDF(&_fontInfo, scale, p_glyph, padding, 128, 0.5f, &width, &height, &xOffset, &yOffset);

		if (glyphBitmap == nullptr)
		{
			_glyphs[p_glyph] = _unknownGlyph;
			return ;
		}

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

		glyph.step = spk::Vector2(static_cast<float>(width), static_cast<float>(height));

		_glyphs[p_glyph] = glyph;

		stbtt_FreeBitmap(glyphBitmap, nullptr);

		_needUpload = true;
		spk::Pipeline::Texture::resetLastActiveTexture();
	}

	void Font::_loadFileData(const std::filesystem::path& p_path)
	{
		_fontData = _readFileContent(p_path);
		stbtt_InitFont(&_fontInfo, _fontData.data(), 0);
	}
}
