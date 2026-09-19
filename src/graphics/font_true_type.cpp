#include "graphics/font.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace spk
{
	struct Font::Resource
	{
		Data data;
		stbtt_fontinfo fontInfo{};

		explicit Resource(Data data);
	};

	namespace
	{
		const std::vector<std::pair<int, int>> UnicodeBlocks = {
			{0x0000, 0x007F},
			{0x0080, 0x00FF}};
	}

	Font::Resource::Resource(Data inputData) :
		data(std::move(inputData))
	{
		if (data.empty())
		{
			throw std::runtime_error("Font can't be initialized from empty data");
		}

		if (stbtt_InitFont(&fontInfo, reinterpret_cast<const unsigned char *>(data.data()), 0) == 0)
		{
			throw std::runtime_error("Font data is not a valid TrueType font");
		}
	}

	void Font::Atlas::loadAllRenderableGlyphs()
	{
		std::unordered_set<int> rendered;

		for (const auto &block : UnicodeBlocks)
		{
			for (int codepoint = block.first; codepoint <= block.second; ++codepoint)
			{
				if (rendered.contains(codepoint))
				{
					continue;
				}

				const int glyphIndex = stbtt_FindGlyphIndex(&_fontResource->fontInfo, codepoint);
				if (glyphIndex != 0 && _glyphs.contains(static_cast<Codepoint>(codepoint)) == false)
				{
					_loadGlyph(static_cast<Codepoint>(codepoint));
					rendered.insert(codepoint);
				}
			}
		}

		if (rendered.empty() == false)
		{
			validate();
			_onEditionContractProvider.trigger();
		}
	}

	Vector2Int Font::Atlas::_computeGlyphPosition(const Vector2UInt &glyphSize)
	{
		if (_nextGlyphAnchor.x + static_cast<int>(glyphSize.x) >=
			_quadrantAnchor.x + static_cast<int>(_quadrantSize.x))
		{
			_nextGlyphAnchor = _nextLineAnchor;
		}

		Vector2Int result = _nextGlyphAnchor;
		_nextGlyphAnchor.x += static_cast<int>(glyphSize.x);
		_nextLineAnchor.y = std::max(_nextLineAnchor.y, result.y + static_cast<int>(glyphSize.y));

		auto overflowQuadrant = [&]() {
			return _nextLineAnchor.y >= _quadrantAnchor.y + static_cast<int>(_quadrantSize.y);
		};

		auto resetToQuadrant = [&](Vector2Int anchor) {
			_quadrantAnchor = anchor;
			result = _nextGlyphAnchor = _nextLineAnchor = anchor;
			_nextGlyphAnchor.x += static_cast<int>(glyphSize.x);
			_nextLineAnchor.y = std::max(_nextLineAnchor.y, result.y + static_cast<int>(glyphSize.y));
		};

		switch (_currentQuadrant)
		{
		case Quadrant::TopLeft:
			if (overflowQuadrant())
			{
				_currentQuadrant = Quadrant::TopRight;
				_resizeData(size() * Vector2UInt{2, 2});
				resetToQuadrant(Vector2Int(static_cast<int>(size().x / 2), 0));
			}
			break;

		case Quadrant::TopRight:
			if (overflowQuadrant())
			{
				_currentQuadrant = Quadrant::DownLeft;
				resetToQuadrant(Vector2Int(0, static_cast<int>(size().y / 2)));
			}
			break;

		case Quadrant::DownLeft:
			if (overflowQuadrant())
			{
				_currentQuadrant = Quadrant::DownRight;
				resetToQuadrant(Vector2Int(static_cast<int>(size().x / 2), static_cast<int>(size().y / 2)));
			}
			break;

		case Quadrant::DownRight:
			if (overflowQuadrant())
			{
				_currentQuadrant = Quadrant::TopRight;
				_resizeData(size() * Vector2UInt{2, 2});
				resetToQuadrant(Vector2Int(static_cast<int>(size().x / 2), 0));
			}
			break;
		}

		return result;
	}

	void Font::Atlas::_loadGlyph(Codepoint codepoint)
	{
		const float scale = stbtt_ScaleForMappingEmToPixels(&_fontResource->fontInfo, static_cast<float>(_textSize));
		const int stbCodepoint = static_cast<int>(codepoint);
		const std::size_t sdfPadding = _outlineSize == 0 ? 0 : _outlineSize + 2;

		int width = 0;
		int height = 0;
		int xOffset = 0;
		int yOffset = 0;

		std::uint8_t *glyphBitmap = stbtt_GetCodepointSDF(
			&_fontResource->fontInfo,
			scale,
			stbCodepoint,
			static_cast<int>(sdfPadding),
			128,
			128.0f / static_cast<float>(sdfPadding == 0 ? 1 : sdfPadding),
			&width,
			&height,
			&xOffset,
			&yOffset);

		if (glyphBitmap == nullptr)
		{
			_glyphs[codepoint] = _unknownGlyph;
			return;
		}

		Glyph glyphData;
		glyphData.size = Vector2UInt(static_cast<unsigned int>(width), static_cast<unsigned int>(height));

		const Vector2Int glyphPosition = _computeGlyphPosition(glyphData.size);
		_applyGlyphPixel(glyphBitmap, glyphPosition, glyphData.size);

		glyphData.baselineOffset = Vector2Int(-xOffset, -yOffset);
		const Vector2 halfPixel = Vector2(0.5f / static_cast<float>(size().x), 0.5f / static_cast<float>(size().y));

		glyphData.positions[0] = Vector2Int(xOffset, yOffset);
		glyphData.positions[1] = Vector2Int(xOffset, yOffset + height);
		glyphData.positions[2] = Vector2Int(xOffset + width, yOffset);
		glyphData.positions[3] = Vector2Int(xOffset + width, yOffset + height);

		glyphData.uvs[0] = Vector2(
			static_cast<float>(glyphPosition.x) / static_cast<float>(size().x) + halfPixel.x,
			static_cast<float>(glyphPosition.y) / static_cast<float>(size().y) + halfPixel.y);
		glyphData.uvs[1] = Vector2(
			static_cast<float>(glyphPosition.x) / static_cast<float>(size().x) + halfPixel.x,
			static_cast<float>(glyphPosition.y + height) / static_cast<float>(size().y) - halfPixel.y);
		glyphData.uvs[2] = Vector2(
			static_cast<float>(glyphPosition.x + width) / static_cast<float>(size().x) - halfPixel.x,
			static_cast<float>(glyphPosition.y) / static_cast<float>(size().y) + halfPixel.y);
		glyphData.uvs[3] = Vector2(
			static_cast<float>(glyphPosition.x + width) / static_cast<float>(size().x) - halfPixel.x,
			static_cast<float>(glyphPosition.y + height) / static_cast<float>(size().y) - halfPixel.y);

		int advance = 0;
		stbtt_GetCodepointHMetrics(&_fontResource->fontInfo, stbCodepoint, &advance, nullptr);
		glyphData.step = Vector2Int(
			static_cast<int>(std::ceil(advance * scale)) + static_cast<int>(_outlineSize),
			0);

		_glyphs[codepoint] = glyphData;
		stbtt_FreeBitmap(glyphBitmap, nullptr);
	}

	void Font::_loadFromData(Data data)
	{
		_atlases.clear();
		_resource = std::make_shared<Resource>(std::move(data));
	}

	void Font::_loadFromFile(const std::filesystem::path &path)
	{
		std::ifstream file(path, std::ios::binary);
		if (file.is_open() == false)
		{
			throw std::runtime_error("Font: failed to open file: " + path.string());
		}

		_loadFromData(Data(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
	}
}
