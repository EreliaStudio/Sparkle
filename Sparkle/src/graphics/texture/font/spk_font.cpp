#include "graphics/texture/font/spk_font.hpp"

namespace spk
{
	void Font::Glyph::rescale(const spk::Vector2& p_scaleRatio)
	{
		for (size_t i = 0; i < 4; i++)
		{
			UVs[i] *= p_scaleRatio;
		}
	}

	void Font::Atlas::_bind(int p_textureIndex) const
	{
		if (_needUpload == true)
		{
			const_cast<Atlas*>(this)->_uploadTexture();
		}
		spk::Texture::_bind(p_textureIndex);
	}

	void Font::Atlas::_rescaleGlyphs(const spk::Vector2& p_scaleRatio)
	{
		for (auto& [key, element] : _glyphs)
		{
			element.rescale(p_scaleRatio);
		}
	}

	void Font::Atlas::_resizeData(const spk::Vector2UInt& p_size)
	{
		std::vector<uint8_t> newPixels(p_size.x * p_size.y, 0x00);
		std::vector<bool> newUsedPixels(p_size.x * p_size.y, false);

		for (size_t x = 0; x < _size.x; x++)
		{
			for (size_t y = 0; y < _size.y; y++)
			{
				newPixels[x + y * p_size.x] = _pixels[x + y * _size.x];
			}
		}

		_rescaleGlyphs(static_cast<spk::Vector2>(_size) / static_cast<spk::Vector2>(p_size));	
		_size = p_size;
		_quadrantSize = _size / 2;
		_pixels.swap(newPixels);
	}

	void Font::Atlas::_applyGlyphPixel(const uint8_t* p_pixelsToApply, const spk::Vector2UInt& p_glyphPosition, const spk::Vector2UInt& p_glyphSize)
	{
		for (size_t x = 0; x < p_glyphSize.x; x++)
		{	
			for (size_t y = 0; y < p_glyphSize.y; y++)
			{
				_pixels[(p_glyphPosition.x + x) + (p_glyphPosition.y + y) * _size.x] = p_pixelsToApply[x + y * p_glyphSize.x];
			}
		}
	}

	void Font::Atlas::_uploadTexture()
	{
		uploadToGPU(
			_pixels.data(),
			_size,
			spk::Texture::Format::GreyLevel,
			spk::Texture::Filtering::Nearest,
			spk::Texture::Wrap::Repeat,
			spk::Texture::Mipmap::Disable
		);
		_needUpload = false;
	}
	
	Font::Atlas::Atlas(const stbtt_fontinfo& p_fontInfo, const std::vector<uint8_t>& p_fontData, const size_t& p_textSize, const size_t& p_outlineSize) :
		_textSize(p_textSize), _outlineSize(p_outlineSize), _fontInfo(p_fontInfo)
	{
		_resizeData(spk::Vector2UInt(1024, 1024));

		_currentQuadrant = Quadrant::TopLeft;
		_quadrantAnchor = spk::Vector2UInt(0, 0);
		_quadrantSize = _size;
		_nextGlyphAnchor = _quadrantAnchor;
		_nextLineAnchor = _quadrantAnchor;
	}

	void Font::Atlas::loadGlyphs(const std::wstring& p_glyphsToLoad)
	{
		for (const auto& c : p_glyphsToLoad)
		{
			operator[](c);
		}
	}

	const Font::Glyph& Font::Atlas::operator[](const wchar_t& p_char)
	{
		if (_glyphs.contains(p_char) == false)
		{
			_loadGlyph(p_char);			
		}
		return _glyphs.at(p_char);
	}

	Font::Font(const std::filesystem::path& p_path)
	{
		_loadFileData(p_path);
	}

	Font::Atlas& Font::atlas(const size_t& p_textSize, const size_t& p_outlineSize)
	{
		auto key = std::make_tuple(p_textSize, p_outlineSize);
		if (_atlases.find(key) == _atlases.end())
		{
			_atlases.emplace(key, Atlas(_fontInfo, _fontData, p_textSize, p_outlineSize));
		}
		return _atlases.at(key);
	}
}