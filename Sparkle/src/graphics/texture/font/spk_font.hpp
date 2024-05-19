#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "graphics/texture/spk_texture.hpp"
#include "external_libraries/stb_truetype.h"

namespace spk
{
	class Font : public spk::Texture
	{
	public:
		struct Glyph
		{
			spk::Vector2UInt positions[4];
			spk::Vector2 UVs[4];
			spk::Vector2 step;

			void rescale(const spk::Vector2& p_scaleRatio);
		};

	private:
		std::unordered_map<wchar_t, Glyph> _glyphs;
		Glyph _unknownGlyph;
		
		std::vector<uint8_t> _fontData;
		stbtt_fontinfo _fontInfo;

		std::vector<uint8_t> _pixels;
		enum class Quadrant
		{
			TopLeft,
			TopRight,
			DownLeft,
			DownRight
		};
		Quadrant _currentQuadrant = Quadrant::TopLeft;
		spk::Vector2UInt _quadrantAnchor = 0;
		spk::Vector2UInt _quadrantSize = 0;
		spk::Vector2UInt _nextGlyphAnchor = 0;
		spk::Vector2UInt _nextLineAnchor = 0;
		spk::Vector2UInt _size;

		bool _needUpload = false;

		void _bind(int p_textureIndex) const override;

		std::vector<uint8_t> _readFileContent(const std::filesystem::path& p_path);

		void _rescaleGlyphs(const spk::Vector2& p_scaleRatio);

		void _resizeData(const spk::Vector2UInt& p_size);

		spk::Vector2UInt _computeGlyphPosition(const spk::Vector2UInt& p_glyphSize);

		void _applyGlyphPixel(const uint8_t* p_pixelsToApply, const spk::Vector2UInt& p_glyphPosition, const spk::Vector2UInt& p_glyphSize);

		void _loadGlyph(const wchar_t& p_glyph);

		void _uploadTexture();

		void _loadFileData(const std::filesystem::path& p_path);

	public:
		Font(const std::filesystem::path& p_path);

		void loadGlyphs(const std::wstring& p_glyphsToLoad);

		const Glyph& operator[](const wchar_t& p_char) const;
	};
}