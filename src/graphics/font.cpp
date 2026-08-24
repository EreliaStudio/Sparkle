#include "graphics/font.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace spk
{
	Font::Text Font::textFromUTF8(std::string_view text)
	{
		static constexpr Codepoint ReplacementCodepoint = U'\uFFFD';

		Text result;
		result.reserve(text.size());

		for (std::size_t i = 0; i < text.size();)
		{
			const auto byte = static_cast<unsigned char>(text[i]);
			if (byte < 0x80)
			{
				result.push_back(static_cast<Codepoint>(byte));
				++i;
				continue;
			}

			Codepoint codepoint = 0;
			std::size_t sequenceLength = 0;
			Codepoint minimumCodepoint = 0;

			if ((byte & 0xE0) == 0xC0)
			{
				codepoint = static_cast<Codepoint>(byte & 0x1F);
				sequenceLength = 2;
				minimumCodepoint = 0x80;
			}
			else if ((byte & 0xF0) == 0xE0)
			{
				codepoint = static_cast<Codepoint>(byte & 0x0F);
				sequenceLength = 3;
				minimumCodepoint = 0x800;
			}
			else if ((byte & 0xF8) == 0xF0)
			{
				codepoint = static_cast<Codepoint>(byte & 0x07);
				sequenceLength = 4;
				minimumCodepoint = 0x10000;
			}
			else
			{
				result.push_back(ReplacementCodepoint);
				++i;
				continue;
			}

			if (i + sequenceLength > text.size())
			{
				result.push_back(ReplacementCodepoint);
				break;
			}

			bool validSequence = true;
			for (std::size_t offset = 1; offset < sequenceLength; ++offset)
			{
				const auto continuation = static_cast<unsigned char>(text[i + offset]);
				if ((continuation & 0xC0) != 0x80)
				{
					validSequence = false;
					break;
				}
				codepoint = static_cast<Codepoint>((codepoint << 6) | (continuation & 0x3F));
			}

			if (validSequence == false ||
				codepoint < minimumCodepoint ||
				codepoint > 0x10FFFF ||
				(codepoint >= 0xD800 && codepoint <= 0xDFFF))
			{
				result.push_back(ReplacementCodepoint);
				++i;
				continue;
			}

			result.push_back(codepoint);
			i += sequenceLength;
		}

		return result;
	}

	void Font::Glyph::rescale(const Vector2 &scaleRatio)
	{
		for (auto &uv : uvs)
		{
			uv *= scaleRatio;
		}
	}

	void Font::Atlas::_rescaleGlyphs(const Vector2 &scaleRatio)
	{
		for (auto &[codepoint, glyphData] : _glyphs)
		{
			(void)codepoint;
			glyphData.rescale(scaleRatio);
		}
	}

	void Font::Atlas::_resizeData(const Vector2UInt &newSize)
	{
		const Vector2UInt previousSize = size();
		resizePixels(newSize);

		_rescaleGlyphs(static_cast<Vector2>(previousSize) / static_cast<Vector2>(newSize));
		_quadrantSize = newSize / Vector2UInt{2, 2};
	}

	void Font::Atlas::_applyGlyphPixel(const std::uint8_t *data, const Vector2Int &position, const Vector2UInt &glyphSize)
	{
		if (position.x < 0 || position.y < 0)
		{
			throw std::logic_error("Font atlas generated a negative glyph position");
		}

		while (position.x + static_cast<int>(glyphSize.x) >= static_cast<int>(size().x) ||
			   position.y + static_cast<int>(glyphSize.y) >= static_cast<int>(size().y))
		{
			_resizeData(size() * Vector2UInt{2, 2});
		}

		writePixels(
			data,
			Vector2UInt(static_cast<unsigned int>(position.x), static_cast<unsigned int>(position.y)),
			glyphSize);
	}

	Font::Atlas::Atlas(std::shared_ptr<Resource> fontResource, std::size_t textSize, std::size_t outlineSize) :
		Texture(Target::Texture2D),
		_fontResource(std::move(fontResource)),
		_textSize(textSize),
		_outlineSize(outlineSize)
	{
		Glyph spaceGlyph;
		spaceGlyph.step = Vector2Int(static_cast<int>(textSize / 2), 0);
		_glyphs[U' '] = spaceGlyph;

		setPixels(nullptr, Vector2UInt(124, 124), Format::GreyLevel);
		setMipmap(Mipmap::Disable);

		_currentQuadrant = Quadrant::TopLeft;
		_quadrantAnchor = Vector2Int(0, 0);
		_quadrantSize = size();
		_nextGlyphAnchor = _quadrantAnchor;
		_nextLineAnchor = _quadrantAnchor;

		validate();
	}

	Font::Atlas::Contract Font::Atlas::subscribe(const Job &job)
	{
		return _onEditionContractProvider.subscribe(job);
	}

	void Font::Atlas::loadGlyphs(const Text &glyphsToLoad)
	{
		for (Codepoint codepoint : glyphsToLoad)
		{
			auto tmpGlyph = glyph(codepoint);
		}
	}

	void Font::Atlas::loadGlyphs(std::string_view utf8GlyphsToLoad)
	{
		loadGlyphs(Font::textFromUTF8(utf8GlyphsToLoad));
	}

	const Font::Glyph &Font::Atlas::operator[](Codepoint codepoint)
	{
		return glyph(codepoint);
	}

	const Font::Glyph &Font::Atlas::glyph(Codepoint codepoint)
	{
		if (_glyphs.contains(codepoint) == false)
		{
			_loadGlyph(codepoint);
			validate();
			_onEditionContractProvider.trigger();
		}

		return _glyphs.at(codepoint);
	}

	Vector2UInt Font::Atlas::computeCharSize(Codepoint codepoint)
	{
		return glyph(codepoint).size;
	}

	Vector2UInt Font::Atlas::computeStringSize(const Text &string)
	{
		int totalWidth = 0;
		int maxHeight = 0;
		int minHeight = 0;

		for (Codepoint codepoint : string)
		{
			const Glyph &glyphData = glyph(codepoint);
			totalWidth += glyphData.step.x;
			maxHeight = std::max(maxHeight, glyphData.positions[3].y);
			minHeight = std::min(minHeight, glyphData.positions[0].y);
		}

		return Vector2UInt(
			static_cast<unsigned int>(totalWidth),
			static_cast<unsigned int>(maxHeight - minHeight));
	}

	Vector2UInt Font::Atlas::computeStringSize(std::string_view utf8String)
	{
		return computeStringSize(Font::textFromUTF8(utf8String));
	}

	Vector2Int Font::Atlas::computeStringBaselineOffset(const Text &string)
	{
		Vector2Int result(0, 0);

		for (std::size_t i = 0; i < string.size(); ++i)
		{
			const Glyph &glyphData = glyph(string[i]);
			if (i == 0)
			{
				result.x = glyphData.baselineOffset.x;
			}
			result.y = std::max(result.y, glyphData.baselineOffset.y);
		}

		return result;
	}

	Vector2Int Font::Atlas::computeStringBaselineOffset(std::string_view utf8String)
	{
		return computeStringBaselineOffset(Font::textFromUTF8(utf8String));
	}

	Font Font::fromRawData(Data data)
	{
		Font result;
		result._loadFromData(std::move(data));
		return result;
	}

	Font::Font(const std::filesystem::path &path)
	{
		_loadFromFile(path);
	}

	Vector2UInt Font::computeCharSize(Codepoint codepoint, const Size &fontSize)
	{
		return atlas(fontSize).computeCharSize(codepoint);
	}

	Vector2UInt Font::computeStringSize(const Text &string, const Size &fontSize)
	{
		return atlas(fontSize).computeStringSize(string);
	}

	Vector2UInt Font::computeStringSize(std::string_view utf8String, const Size &fontSize)
	{
		return computeStringSize(textFromUTF8(utf8String), fontSize);
	}

	Vector2Int Font::computeStringBaselineOffset(const Text &string, const Size &fontSize)
	{
		return atlas(fontSize).computeStringBaselineOffset(string);
	}

	Vector2Int Font::computeStringBaselineOffset(std::string_view utf8String, const Size &fontSize)
	{
		return computeStringBaselineOffset(textFromUTF8(utf8String), fontSize);
	}

	Font::Size Font::computeOptimalTextSize(const Text &string, float outlineSizeRatio, const Vector2UInt &textArea)
	{
		static const std::vector<std::size_t> deltas = {100u, 50u, 20u, 10u, 1u};
		Size result(2, 0);

		if (string.empty())
		{
			const std::size_t outlineSize = static_cast<std::size_t>(textArea.y * outlineSizeRatio);
			return Size(textArea.y - outlineSize * 2, outlineSize);
		}

		for (std::size_t delta : deltas)
		{
			if (delta > textArea.y)
			{
				continue;
			}

			bool enough = false;
			while (enough == false)
			{
				const std::size_t glyphSize = result.glyph + delta;
				const std::size_t outlineSize = static_cast<std::size_t>(glyphSize * outlineSizeRatio);
				const Vector2UInt temporarySize = computeStringSize(string, {glyphSize - outlineSize * 2, outlineSize});

				if (temporarySize.x >= textArea.x || temporarySize.y >= textArea.y)
				{
					enough = true;
				}
				else
				{
					result.glyph += delta;
				}
			}
		}

		result.outline = static_cast<std::size_t>(result.glyph * outlineSizeRatio);
		result.glyph -= result.outline * 2;

		return result;
	}

	Font::Size Font::computeOptimalTextSize(std::string_view utf8String, float outlineSizeRatio, const Vector2UInt &textArea)
	{
		return computeOptimalTextSize(textFromUTF8(utf8String), outlineSizeRatio, textArea);
	}

	Font::Atlas &Font::atlas(const Size &fontSize)
	{
		if (_resource == nullptr)
		{
			throw std::logic_error("Cannot create a Font atlas without loaded font data");
		}

		auto it = _atlases.find(fontSize);
		if (it == _atlases.end())
		{
			auto atlasEntry = std::unique_ptr<Atlas>(new Atlas(_resource, fontSize.glyph, fontSize.outline));
			it = _atlases.emplace(fontSize, std::move(atlasEntry)).first;
		}

		return *it->second;
	}
}
