#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>
#include "structure/graphics/opengl/spk_texture_object.hpp"
#include "external_libraries/stb_truetype.h"

namespace spk
{
	/**
	 * @class Font
	 * @brief Represents a font for rendering text, including glyph management and atlas handling.
	 * 
	 * The `Font` class handles loading font data, creating atlases for different sizes, and computing
	 * text and character dimensions. It supports scaling, glyph retrieval, and text rendering utilities.
	 * 
	 * Example usage:
	 * @code
	 * spk::Font font("path/to/font.ttf");
	 * spk::Font::Atlas& atlas = font.atlas({32, 0});
	 * atlas.loadGlyphs(L"Hello, World!");
	 * auto size = font.computeStringSize("Hello", 32, 0);
	 * @endcode
	 */
	class Font
	{
	public:
		using Data = std::vector<uint8_t>; ///< Represents raw font data.

		/**
		 * @struct Glyph
		 * @brief Represents a single character's rendering data.
		 */
		struct Glyph
		{
			Vector2Int positions[4]; ///< Vertex positions for the glyph.
			Vector2 UVs[4]; ///< UV coordinates for the glyph.
			Vector2Int step; ///< Step size for rendering the glyph.
			Vector2UInt size; ///< Size of the glyph in pixels.

			/**
			 * @brief Defines the default order of indices for rendering triangles.
			 */
			static inline std::vector<unsigned int> indexesOrder = { 0, 1, 2, 2, 1, 3 };

			/**
			 * @brief Rescales the glyph by a specified scale ratio.
			 * @param p_scaleRatio The scaling ratio.
			 */
			void rescale(const Vector2& p_scaleRatio);
		};

		/**
		 * @struct Size
		 * @brief Represents the text and outline size for rendering.
		 */
		struct Size
		{
			size_t text; ///< Text size in pixels.
			size_t outline; ///< Outline size in pixels.

			/**
			 * @brief Default constructor.
			 */
			Size();

			/**
			 * @brief Constructs a size with text size only.
			 * @param p_text Text size in pixels.
			 */
			Size(size_t p_text);

			/**
			 * @brief Constructs a size with text and outline sizes.
			 * @param p_text Text size in pixels.
			 * @param p_outline Outline size in pixels.
			 */
			Size(size_t p_text, size_t p_outline);

			bool operator<(const Size& p_other) const;
			friend std::wostream& operator<<(std::wostream& p_os, const Size& size);
			friend std::ostream& operator<<(std::ostream& p_os, const Size& size);
		};

		/**
		 * @class Atlas
		 * @brief Manages a texture atlas for a specific font size and outline.
		 * 
		 * The `Atlas` class handles loading glyphs, computing their positions, and uploading them
		 * as textures for efficient rendering.
		 */
		class Atlas : public OpenGL::TextureObject
		{
			friend class Font;

		private:
			const stbtt_fontinfo& _fontInfo; ///< Reference to the font info.
			std::unordered_map<wchar_t, Glyph> _glyphs; ///< Map of glyphs for characters.
			Glyph _unknownGlyph; ///< Fallback glyph for unknown characters.

			std::vector<uint8_t> _pixels; ///< Pixel data for the atlas texture.
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

			Atlas(const stbtt_fontinfo& p_fontInfo, const Data& p_fontData, const size_t& p_textSize, const size_t& p_outlineSize);

		public:
			/**
			 * @brief Loads specified glyphs into the atlas.
			 * @param p_glyphsToLoad The characters to load.
			 */
			void loadGlyphs(const std::wstring& p_glyphsToLoad);

			/**
			 * @brief Loads all renderable glyphs from the font.
			 */
			void loadAllRenderableGlyphs();

			/**
			 * @brief Accesses a glyph by character.
			 * @param p_char The character for the glyph.
			 * @return A reference to the corresponding glyph.
			 */
			const Glyph& operator[](const wchar_t& p_char);

			/**
			 * @brief Retrieves a glyph by character.
			 * @param p_char The character for the glyph.
			 * @return A reference to the corresponding glyph.
			 */
			const Glyph& glyph(const wchar_t& p_char);

			/**
			 * @brief Computes the size of a single character.
			 * @param p_char The character to measure.
			 * @return The size of the character in pixels.
			 */
			Vector2UInt computeCharSize(const wchar_t& p_char);

			/**
			 * @brief Computes the size of a string.
			 * @param p_string The string to measure.
			 * @return The size of the string in pixels.
			 */
			Vector2UInt computeStringSize(const std::string& p_string);
		};

	private:
		void _loadFileData(const std::filesystem::path& p_path);

		std::map<Size, Atlas> _atlases; ///< Map of atlases for different font sizes.
		Data _fontData; ///< Raw font data.
		stbtt_fontinfo _fontInfo; ///< Font info structure.

	public:
		/**
		 * @brief Constructs a `Font` object from a font file.
		 * @param p_path Path to the font file.
		 */
		Font(const std::filesystem::path& p_path);

		/**
		 * @brief Computes the size of a character for a specific text and outline size.
		 * @param p_char The character to measure.
		 * @param p_size The text size in pixels.
		 * @param p_outlineSize The outline size in pixels.
		 * @return The size of the character in pixels.
		 */
		Vector2UInt computeCharSize(const wchar_t& p_char, size_t p_size, size_t p_outlineSize);

		/**
		 * @brief Computes the size of a string for a specific text and outline size.
		 * @param p_string The string to measure.
		 * @param p_size The text size in pixels.
		 * @param p_outlineSize The outline size in pixels.
		 * @return The size of the string in pixels.
		 */
		Vector2UInt computeStringSize(const std::string& p_string, size_t p_size, size_t p_outlineSize);

		/**
		 * @brief Computes the optimal text size to fit within a specified area.
		 * @param p_string The string to measure.
		 * @param p_outlineSizeRatio The ratio of outline size to text size.
		 * @param p_textArea The available area for the text.
		 * @return The optimal text size and outline size.
		 */
		Size computeOptimalTextSize(const std::string& p_string, float p_outlineSizeRatio, const Vector2UInt& p_textArea);

		/**
		 * @brief Retrieves an atlas for a specific size.
		 * @param p_size The text and outline size.
		 * @return A reference to the atlas.
		 */
		Atlas& atlas(const Size& p_size);
	};
}
