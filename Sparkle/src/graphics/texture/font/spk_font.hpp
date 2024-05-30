#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "graphics/texture/spk_texture.hpp"
#include "external_libraries/stb_truetype.h"

namespace spk
{
	/**
	 * @class Font
	 * @brief Manages font loading, glyph handling, and text rendering.
	 *
	 * The Font class provides functionalities to load font data from files, manage glyphs, and render text. It supports text size and outline size customization, glyph rescaling, and texture atlas creation.
	 * 
	 * Usage example:
	 * @code
	 * spk::Font myFont("path/to/font.ttf");
	 * spk::Vector2Int charSize = myFont.computeCharSize(L'A', 16, 1);
	 * spk::Font::Atlas& atlas = myFont.atlas(16, 1);
	 * const spk::Font::Glyph& glyph = atlas[L'A'];
	 * @endcode
	 *
	 * @note Ensure the file path provided to the constructor points to a valid font file. The class relies on stb_truetype for font handling.
	 */
	class Font
	{
	public:
		/**
		 * @struct Glyph
		 * @brief Represents a single glyph with its properties.
		 *
		 * This structure holds the positions, UVs, step size, and dimensions of a glyph. It also provides a method to rescale glyph dimensions.
		 */
		struct Glyph
		{
			spk::Vector2Int positions[4]; ///< The positions of the glyph's corners.
			spk::Vector2 UVs[4]; ///< The UV coordinates of the glyph's texture.
			spk::Vector2Int step; ///< The step size for the glyph.
			spk::Vector2Int size; ///< The dimensions of the glyph.
			static inline std::vector<unsigned int> indexesOrder = {0, 1, 2, 2, 1, 3}; ///< The order of vertex indexes for rendering.

			/**
			 * @brief Rescales the glyph based on the provided scale ratio.
			 * @param p_scaleRatio The scale ratio to apply to the glyph.
			 */
			void rescale(const spk::Vector2& p_scaleRatio);
		};

		/**
		 * @struct Size
		 * @brief Represents the size of text and its outline.
		 *
		 * This structure holds the text size and outline size, providing multiple constructors for different initializations.
		 */
		struct Size
		{
			size_t text; ///< The size of the text.
			size_t outline; ///< The size of the outline.

			/**
			 * @brief Default constructor.
			 * 
			 * Initializes the text size and outline size to 0.
			 */
			Size() :
				text(0),
				outline(0)
			{

			}

			/**
			 * @brief Constructor with text size.
			 * 
			 * Initializes the text size to the specified value and the outline size to 0.
			 * 
			 * @param p_text The size of the text.
			 */
			Size(size_t p_text) :
				text(p_text),
				outline(0)
			{

			}

			/**
			 * @brief Constructor with text and outline size.
			 * 
			 * Initializes the text size and outline size to the specified values.
			 * 
			 * @param p_text The size of the text.
			 * @param p_outline The size of the outline.
			 */
			Size(size_t p_text, size_t p_outline) :
				text(p_text),
				outline(p_outline)
			{

			}
		};

		/**
		 * @class Atlas
		 * @brief Manages a texture atlas for font rendering.
		 *
		 * The Atlas class handles the creation and management of a texture atlas for storing glyphs. It supports loading glyphs, rescaling, and uploading texture data to the GPU.
		 */
		class Atlas : public spk::Texture
		{
			friend class Font;

		private:
			const stbtt_fontinfo& _fontInfo; ///< The font information.
			std::unordered_map<wchar_t, Glyph> _glyphs; ///< The map of glyphs.
			Glyph _unknownGlyph; ///< The glyph for unknown characters.

			std::vector<uint8_t> _pixels; ///< The pixel data for the texture atlas.
			enum class Quadrant
			{
				TopLeft,
				TopRight,
				DownLeft,
				DownRight
			};
			Quadrant _currentQuadrant = Quadrant::TopLeft; ///< The current quadrant being filled.
			spk::Vector2UInt _quadrantAnchor = 0; ///< The anchor position for the current quadrant.
			spk::Vector2UInt _quadrantSize = 0; ///< The size of the current quadrant.
			spk::Vector2UInt _nextGlyphAnchor = 0; ///< The anchor position for the next glyph.
			spk::Vector2UInt _nextLineAnchor = 0; ///< The anchor position for the next line.
			spk::Vector2UInt _size; ///< The size of the atlas.

			bool _needUpload = false; ///< Flag indicating if the texture needs to be uploaded.

			size_t _textSize; ///< The size of the text.
			size_t _outlineSize; ///< The size of the outline.

			void _bind(int p_textureIndex) const;

			void _rescaleGlyphs(const spk::Vector2& p_scaleRatio);

			void _resizeData(const spk::Vector2UInt& p_size);

			spk::Vector2UInt _computeGlyphPosition(const spk::Vector2UInt& p_glyphSize);

			void _applyGlyphPixel(const uint8_t* p_pixelsToApply, const spk::Vector2UInt& p_glyphPosition, const spk::Vector2UInt& p_glyphSize);

			void _loadGlyph(const wchar_t& p_char);

			void _uploadTexture();

			Atlas(const stbtt_fontinfo& p_fontInfo, const std::vector<uint8_t>& p_fontData, const size_t& p_textSize, const size_t& p_outlineSize);
		public:
			/**
			 * @brief Loads specified glyphs into the atlas.
			 * @param p_glyphsToLoad A string of glyphs to load.
			 */
			void loadGlyphs(const std::wstring& p_glyphsToLoad);

			/**
			 * @brief Loads all renderable glyphs into the atlas.
			 */
			void loadAllRenderableGlyphs();

			/**
			 * @brief Retrieves the glyph for a specific character.
			 * @param p_char The character to retrieve the glyph for.
			 * @return The glyph for the specified character.
			 */
			const Glyph& operator[](const wchar_t& p_char);

			/**
			 * @brief Retrieves the glyph for a specific character.
			 * @param p_char The character to retrieve the glyph for.
			 * @return The glyph for the specified character.
			 */
			const Glyph& glyph(const wchar_t& p_char);

			/**
			 * @brief Computes the size of a character.
			 * @param p_char The character to compute the size for.
			 * @return The size of the character.
			 */
			Vector2Int computeCharSize(const wchar_t& p_char);

			/**
			 * @brief Computes the size of a string.
			 * @param p_string The string to compute the size for.
			 * @return The size of the string.
			 */
			Vector2Int computeStringSize(const std::string& p_string);
		};

	private:

		std::vector<uint8_t> _readFileContent(const std::filesystem::path& p_path);
		void _loadFileData(const std::filesystem::path& p_path);

		std::map<std::tuple<size_t, size_t>, Atlas> _atlases;
		std::vector<uint8_t> _fontData;
		stbtt_fontinfo _fontInfo;

	public:
		/**
		 * @brief Constructor for the Font class.
		 * 
		 * This constructor takes a filesystem path to a font file, reads the file, and initializes the Font object with its data. The font data includes glyph information, size, and outline details.
		 * 
		 * @param p_path The filesystem path to the font file. It should be a valid path to a font file that the system can read and decode.
		 */
		Font(const std::filesystem::path& p_path);

		/**
		 * @brief Computes the size of a character for a specific text size and outline size.
		 * @param p_char The character to compute the size for.
		 * @param p_size The size of the text.
		 * @param p_outlineSize The size of the outline.
		 * @return The size of the character.
		 */
		Vector2Int computeCharSize(const wchar_t& p_char, size_t p_size, size_t p_outlineSize);

		/**
		 * @brief Computes the size of a string for a specific text size and outline size.
		 * @param p_string The string to compute the size for.
		 * @param p_size The size of the text.
		 * @param p_outlineSize The size of the outline.
		 * @return The size of the string.
		 */
		Vector2Int computeStringSize(const std::string& p_string, size_t p_size, size_t p_outlineSize);

		/**
		 * @brief Computes the optimal text size based on the string, outline size ratio, and text area.
		 * @param p_string The string to compute the optimal text size for.
		 * @param p_outlineSizeRatio The ratio of the outline size to the text size.
		 * @param p_textArea The text area dimensions.
		 * @return The optimal text size.
		 */
		Size computeOptimalTextSize(const std::string& p_string, float p_outlineSizeRatio, const Vector2Int& p_textArea);

		/**
		 * @brief Retrieves the atlas for a specific text size and outline size.
		 * @param p_textSize The size of the text.
		 * @param p_outlineSize The size of the outline.
		 * @return The atlas for the specified text and outline sizes.
		 */
		Atlas& atlas(const size_t& p_textSize, const size_t& p_outlineSize);
	};
}
