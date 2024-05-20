#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "graphics/texture/spk_texture.hpp"
#include "external_libraries/stb_truetype.h"

namespace spk
{
	class Font
	{
	public:
		struct Glyph
		{
			spk::Vector2Int positions[4];
			spk::Vector2 UVs[4];
			spk::Vector2Int step;
			static inline std::vector<unsigned int> indexesOrder = {0, 1, 2, 2, 1, 3};

			void rescale(const spk::Vector2& p_scaleRatio);
		};

		class Atlas : public spk::Texture
		{
			friend class Font;

		private:
			const stbtt_fontinfo& _fontInfo;
			std::unordered_map<wchar_t, Glyph> _glyphs;
			Glyph _unknownGlyph;

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

			size_t _textSize;
			size_t _outlineSize;

			void _bind(int p_textureIndex) const;

			void _rescaleGlyphs(const spk::Vector2& p_scaleRatio);

			void _resizeData(const spk::Vector2UInt& p_size);

			spk::Vector2UInt _computeGlyphPosition(const spk::Vector2UInt& p_glyphSize);

			void _applyGlyphPixel(const uint8_t* p_pixelsToApply, const spk::Vector2UInt& p_glyphPosition, const spk::Vector2UInt& p_glyphSize);

			void _loadGlyph(const wchar_t& p_char);

			void _uploadTexture();

			Atlas(const stbtt_fontinfo& p_fontInfo, const std::vector<uint8_t>& p_fontData, const size_t& p_textSize, const size_t& p_outlineSize);
		public:
			void loadGlyphs(const std::wstring& p_glyphsToLoad);
			void loadAllRenderableGlyphs();
			const Glyph& operator[](const wchar_t& p_char);
			const Glyph& glyph(const wchar_t& p_char);
		};

	private:

		std::vector<uint8_t> _readFileContent(const std::filesystem::path& p_path);
		void _loadFileData(const std::filesystem::path& p_path);

		std::map<std::tuple<size_t, size_t>, Atlas> _atlases;
		std::vector<uint8_t> _fontData;
		stbtt_fontinfo _fontInfo;

	public:
		Font(const std::filesystem::path& p_path);

		Atlas& atlas(const size_t& p_textSize, const size_t& p_outlineSize);
	};
}