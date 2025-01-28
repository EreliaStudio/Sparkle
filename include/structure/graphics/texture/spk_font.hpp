#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "structure/graphics/opengl/spk_texture_object.hpp"
#include "external_libraries/stb_truetype.h"

#include "structure/graphics/spk_text_alignment.hpp"

namespace spk
{
	class Font
	{
	public:
		using Data = std::vector<uint8_t>;
		using Filtering = OpenGL::TextureObject::Filtering;
		using Wrap = OpenGL::TextureObject::Wrap;
		using Mipmap = OpenGL::TextureObject::Mipmap;

		struct Glyph
		{
			Vector2Int positions[4];
			Vector2 UVs[4];
			Vector2Int step;
			Vector2Int baselineOffset;
			Vector2UInt size;
			static inline std::vector<unsigned int> indexesOrder = { 0, 1, 2, 2, 1, 3 };

			void rescale(const Vector2& p_scaleRatio);
		};

		struct Size
		{
			size_t text;
			size_t outline;

			Size() :
				text(0),
				outline(0)
			{

			}

			Size(size_t p_text) :
				text(p_text),
				outline(0)
			{

			}

			Size(size_t p_text, size_t p_outline) :
				text(p_text),
				outline(p_outline)
			{

			}

			bool operator<(const Size& other) const
			{
				if (text < other.text)
				{
					return true;
				}
				if (text > other.text)
				{
					return false;
				}
				return outline < other.outline;
			}

			friend std::wostream& operator<<(std::wostream& p_os, const Size& size)
			{
				p_os << L"(" << size.text << L" / " << size.outline << L")";
				return p_os;
			}

			friend std::ostream& operator<<(std::ostream& p_os, const Size& size)
			{
				p_os << "(" << size.text << " / " << size.outline << ")";
				return p_os;
			}
		};

		class Atlas : public OpenGL::TextureObject
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
			Vector2Int _quadrantAnchor = 0;
			Vector2UInt _quadrantSize = 0;
			Vector2Int _nextGlyphAnchor = 0; 
			Vector2Int _nextLineAnchor = 0;
			Vector2UInt _size;

			bool _needUpload = false;

			size_t _textSize;
			size_t _outlineSize;

			void _rescaleGlyphs(const Vector2& p_scaleRatio);

			void _resizeData(const Vector2UInt& p_size);

			Vector2Int _computeGlyphPosition(const Vector2UInt& p_glyphSize);

			void _applyGlyphPixel(const uint8_t* p_pixelsToApply, const Vector2Int& p_glyphPosition, const Vector2UInt& p_glyphSize);

			void _loadGlyph(const wchar_t& p_char);

			void _uploadTexture();

			Atlas(const stbtt_fontinfo& p_fontInfo, const Data& p_fontData, const size_t& p_textSize, const size_t& p_outlineSize, const Filtering& p_filtering = Filtering::Linear,
			const Wrap& p_wrap = Wrap::Repeat, const Mipmap& p_mipmap = Mipmap::Disable);
		public:
			void loadGlyphs(const std::wstring& p_glyphsToLoad);

			void loadAllRenderableGlyphs();

			const Glyph& operator[](const wchar_t& p_char);

			const Glyph& glyph(const wchar_t& p_char);

			Vector2UInt computeCharSize(const wchar_t& p_char);

			Vector2UInt computeStringSize(const std::wstring& p_string);
			Vector2Int computeStringBaselineOffset(const std::wstring& p_string);
			Vector2Int computeStringAnchor(const spk::Geometry2D& p_geometry, const std::wstring& p_string, spk::HorizontalAlignment p_horizontalAlignment, spk::VerticalAlignment p_verticalAlignment);

		};

	private:
		void loadFromFile(const std::filesystem::path& p_path);
		void loadFromData(const std::vector<uint8_t>& p_data);

		std::map<Size, Atlas> _atlases;
		Data _fontData;
		stbtt_fontinfo _fontInfo;

		Filtering _filtering = Filtering::Linear;
		Wrap _wrap = Wrap::Repeat;
		Mipmap _mipmap = Mipmap::Enable;

	public:
		static Font fromRawData(const std::vector<uint8_t>& p_data,
			const Filtering& p_filtering = Filtering::Linear,
			const Wrap& p_wrap = Wrap::Repeat, const Mipmap& p_mipmap = Mipmap::Enable);

		Font();
		Font(const std::filesystem::path& p_path);

		void setProperties(
			const Filtering& p_filtering,
			const Wrap& p_wrap,
			const Mipmap& p_mipmap
			);

		Vector2UInt computeCharSize(const wchar_t& p_char, const Font::Size& p_size);

		Vector2UInt computeStringSize(const std::wstring& p_string, const Font::Size& p_size);
		Vector2Int computeStringBaselineOffset(const std::wstring& p_string, const Font::Size& p_size);
		Vector2Int computeStringAnchor(const spk::Geometry2D& p_geometry, const std::wstring& p_string, const Font::Size& p_size, spk::HorizontalAlignment p_horizontalAlignment, spk::VerticalAlignment p_verticalAlignment);

		Size computeOptimalTextSize(const std::wstring& p_string, float p_outlineSizeRatio, const Vector2UInt& p_textArea);

		Atlas& atlas(const Size& p_size);
	};
}
