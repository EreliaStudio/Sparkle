#include "structure/graphics/texture/spk_font.hpp"

#include "structure/spk_iostream.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	void Font::Glyph::rescale(const Vector2 &p_scaleRatio)
	{
		for (size_t i = 0; i < 4; i++)
		{
			uvs[i] *= p_scaleRatio;
		}
	}

	void Font::Atlas::_rescaleGlyphs(const Vector2 &p_scaleRatio)
	{
		for (auto &[key, element] : _glyphs)
		{
			element.rescale(p_scaleRatio);
		}
	}

	void Font::Atlas::_resizeData(const Vector2UInt &p_size)
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

		_rescaleGlyphs(static_cast<Vector2>(_size) / static_cast<Vector2>(p_size));
		_size = p_size;
		_quadrantSize = _size / 2;
		_pixels.swap(newPixels);
	}

	void Font::Atlas::_applyGlyphPixel(const uint8_t *p_pixelsToApply, const Vector2Int &p_glyphPosition, const Vector2UInt &p_glyphSize)
	{
		while ((p_glyphPosition.x + p_glyphSize.x >= _size.x) || (p_glyphPosition.y + p_glyphSize.y >= _size.y))
		{
			_resizeData(_size * 2);
		}
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
		setPixels(_pixels.data(), _size, Texture::Format::GreyLevel);
	}

	Font::Atlas::Atlas(
		const stbtt_fontinfo &p_fontInfo,
		const Data &p_fontData,
		const size_t &p_textSize,
		const size_t &p_outlineSize,
		const Filtering &p_filtering,
		const Wrap &p_wrap,
		const Mipmap &p_mipmap) :
		_textSize(p_textSize),
		_outlineSize(p_outlineSize),
		_fontInfo(p_fontInfo)
	{
		Font::Glyph spaceGlyph;

		spaceGlyph.step = Vector2Int(p_textSize / 2.0f, 0);

		setProperties(p_filtering, p_wrap, p_mipmap);

		_glyphs[L' '] = spaceGlyph;
		_resizeData(Vector2UInt(124, 124));

		_currentQuadrant = Quadrant::TopLeft;
		_quadrantAnchor = Vector2Int(0, 0);
		_quadrantSize = _size;
		_nextGlyphAnchor = _quadrantAnchor;
		_nextLineAnchor = _quadrantAnchor;
	}

	void Font::Atlas::loadGlyphs(const std::wstring &p_glyphsToLoad)
	{
		for (const auto &c : p_glyphsToLoad)
		{
			operator[](c);
		}
	}

	Font::Atlas::Contract Font::Atlas::subscribe(const Font::Atlas::Job &p_job)
	{
		return _onEditionContractProvider.subscribe(p_job);
	}

	const Font::Glyph &Font::Atlas::operator[](const wchar_t &p_char)
	{
		return (glyph(p_char));
	}

	const Font::Glyph &Font::Atlas::glyph(const wchar_t &p_char)
	{
		if (_glyphs.contains(p_char) == false)
		{
			_loadGlyph(p_char);

			_uploadTexture();

			_onEditionContractProvider.trigger();
		}
		return _glyphs.at(p_char);
	}

	Font Font::fromRawData(const std::vector<uint8_t> &p_data, const Filtering &p_filtering, const Wrap &p_wrap, const Mipmap &p_mipmap)
	{
		Font result;

		result._loadFromData(p_data);
		result.setProperties(p_filtering, p_wrap, p_mipmap);

		return (result);
	}

	Font::Font()
	{
	}

	Font::Font(const std::filesystem::path &p_path)
	{
		_loadFromFile(p_path);
	}

	void Font::setProperties(const Filtering &p_filtering, const Wrap &p_wrap, const Mipmap &p_mipmap)
	{
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;

		for (auto &atlas : _atlases)
		{
			atlas.second.setProperties(_filtering, _wrap, _mipmap);
		}
	}

	Vector2UInt Font::Atlas::computeCharSize(const wchar_t &p_char)
	{
		return (operator[](p_char).size);
	}

	Vector2UInt Font::Atlas::computeStringSize(const std::wstring &p_string)
	{
		int totalWidth = 0;
		int maxHeight = 0;
		int minHeight = 0;

		for (size_t i = 0; i < p_string.size(); i++)
		{
			const Glyph &glyph = operator[](p_string[i]);

			totalWidth += glyph.step.x;

			maxHeight = std::max(maxHeight, glyph.positions[3].y);
			minHeight = std::min(minHeight, glyph.positions[0].y);
		}

		int totalHeight = maxHeight - minHeight;
		return Vector2UInt(totalWidth, totalHeight);
	}

	Vector2Int Font::Atlas::computeStringBaselineOffset(const std::wstring &p_string)
	{
		Vector2Int baselineResult = 0;

		for (size_t i = 0; i < p_string.size(); i++)
		{
			const Glyph &glyph = operator[](p_string[i]);

			if (i == 0)
			{
				baselineResult.x = glyph.baselineOffset.x;
			}
			baselineResult.y = std::max(baselineResult.y, glyph.baselineOffset.y);
		}

		return (baselineResult);
	}

	Vector2Int Font::Atlas::computeStringAnchor(
		const spk::Geometry2D &p_geometry,
		const std::wstring &p_string,
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment)
	{
		spk::Vector2Int result = p_geometry.anchor;

		switch (p_horizontalAlignment)
		{
		case HorizontalAlignment::Left:
		{
			spk::Vector2Int stringBaseline = computeStringBaselineOffset(p_string);

			result.x += stringBaseline.x;
			break;
		}
		case HorizontalAlignment::Centered:
		{
			spk::Vector2Int stringSize = computeStringSize(p_string);
			spk::Vector2Int stringBaseline = computeStringBaselineOffset(p_string);

			result.x += p_geometry.size.x / 2 + (stringBaseline.x) - stringSize.x / 2;
			break;
		}
		case HorizontalAlignment::Right:
		{
			spk::Vector2Int stringSize = computeStringSize(p_string);
			spk::Vector2Int stringBaseline = computeStringBaselineOffset(p_string);

			result.x += p_geometry.size.x + stringBaseline.x - stringSize.x;
			break;
		}
		}

		switch (p_verticalAlignment)
		{
		case VerticalAlignment::Top:
		{
			spk::Vector2Int stringBaseline = computeStringBaselineOffset(p_string);

			result.y += stringBaseline.y;
			break;
		}
		case VerticalAlignment::Centered:
		{
			spk::Vector2Int stringSize = computeStringSize(p_string);
			spk::Vector2Int stringBaseline = computeStringBaselineOffset(p_string);

			result.y += p_geometry.size.y / 2 + (stringBaseline.y) - stringSize.y / 2;
			break;
		}
		case VerticalAlignment::Down:
		{
			spk::Vector2Int stringSize = computeStringSize(p_string);
			spk::Vector2Int stringBaseline = computeStringBaselineOffset(p_string);

			result.y += p_geometry.size.y + (stringBaseline.y) - stringSize.y;
			break;
		}
		}

		return (result);
	}

	Vector2UInt Font::computeCharSize(const wchar_t &p_char, const Font::Size &p_size)
	{
		return (atlas(p_size).computeCharSize(p_char));
	}

	Vector2UInt Font::computeStringSize(const std::wstring &p_string, const Font::Size &p_size)
	{
		return (atlas(p_size).computeStringSize(p_string));
	}

	Vector2Int Font::computeStringBaselineOffset(const std::wstring &p_string, const Font::Size &p_size)
	{
		return (atlas(p_size).computeStringBaselineOffset(p_string));
	}

	Vector2Int Font::computeStringAnchor(
		const spk::Geometry2D &p_geometry,
		const std::wstring &p_string,
		const Font::Size &p_size,
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment)
	{
		return (atlas(p_size).computeStringAnchor(p_geometry, p_string, p_horizontalAlignment, p_verticalAlignment));
	}

	Font::Size Font::computeOptimalTextSize(const std::wstring &p_string, float p_outlineSizeRatio, const Vector2UInt &p_textArea)
	{
		std::vector<size_t> deltas = {100u, 50u, 20u, 10u, 1u};
		Font::Size result = 2;

		if (p_string.empty())
		{
			size_t resultTextSize = p_textArea.y;
			size_t resultOutlineSize = static_cast<size_t>(resultTextSize * p_outlineSizeRatio);
			resultTextSize -= resultOutlineSize * 2;

			return (Font::Size(resultTextSize, resultOutlineSize));
		}

		for (int i = 0; i < static_cast<int>(deltas.size()); i++)
		{
			if (deltas[i] > p_textArea.y)
			{
				continue;
			}

			bool enough = false;
			while (enough == false)
			{
				size_t glyphSize = result.glyph + deltas[i];
				size_t outlineSize = static_cast<size_t>(glyphSize * p_outlineSizeRatio);
				glyphSize -= outlineSize * 2;

				Vector2UInt tmpSize = computeStringSize(p_string, {glyphSize, outlineSize});

				if (tmpSize.x >= p_textArea.x || tmpSize.y >= p_textArea.y)
				{
					enough = true;
				}
				else
				{
					result.glyph += deltas[i];
				}
			}
		}

		result.outline = static_cast<size_t>(result.glyph * p_outlineSizeRatio);
		result.glyph -= result.outline * 2;

		return (result);
	}

	Font::Atlas &Font::atlas(const Size &p_size)
	{
		if (_atlases.find(p_size) == _atlases.end())
		{
			_atlases.emplace(p_size, std::move(Atlas(_fontInfo, _fontData, p_size.glyph, p_size.outline, _filtering, _wrap, _mipmap)));
		}
		return _atlases.at(p_size);
	}
}