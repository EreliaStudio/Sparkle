#pragma once

#include <vector>
#include <map>
#include <filesystem>

#include "graphics/texture/spk_texture.hpp"

namespace spk
{
    /**
     * @class Font
     * @brief Handles font rendering by managing glyph textures and dynamic font atlas creation.
     *
     * The Font class is designed for flexible and efficient text rendering in graphical applications.
     * It supports the generation of font atlases from font files, allowing for customized text rendering
     * with support for different font sizes, outline styles, and alignment options. This class
     * facilitates the creation of texture atlases for glyphs, optimizing text rendering by batching draw
     * calls.
     *
     * Font atlases are dynamically generated based on the specific requirements of the text to be rendered,
     * such as font size and outline style. The class provides mechanisms to access glyph data for individual
     * characters, including their texture coordinates and positioning information.
     *
     * Usage example:
     * @code
     * spk::Font myFont("path/to/fontfile.ttf");
     * spk::Font::Atlas atlas = myFont.atlas(16, 1); // Get or create a font atlas for font size 16 with 1px outline
     * spk::Font::Atlas::GlyphData glyphE = atlas.glyph('E');
     * @endcode
     *
     * @note The class assumes the font file provided at construction exists and is valid. Atlases created are managed internally and should be accessed via the provided methods rather than directly to ensure thread safety and resource management.
     *
     * @see Atlas, Configuration, Key
     */
    class Font
    {
    public:
        static const uint8_t CHAR_PIXEL = 0xFF;
        static const uint8_t EMPTY_PIXEL = 0x00;

        enum class OutlineStyle
        {
            Pixelized,
            SharpEdge,
            Standard,
            Manhattan
        };

		enum class VerticalAlignment
		{
            Top,
            Middle,
            Down
        };

        enum class HorizontalAlignment
		{
            Left,
            Middle,
            Right
        };

    public:
        /**
         * @class Configuration
         * @brief Holds configuration data for font loading and glyph validation.
         *
         * This nested class within Font is responsible for storing configuration details related to a specific
         * font file. It includes the path to the font file and a list of valid glyphs that can be rendered.
         * The Configuration class plays a crucial role in pre-processing font data, ensuring that only supported
         * glyphs are considered when generating font atlases.
         *
         * The constructor initializes the configuration with the file path and font data, automatically computing
         * glyph information to filter out unsupported characters based on the font's capabilities.
         *
         * Usage example: Not directly instantiated by users, used internally by Font.
         *
         * @see Font, Atlas
         */
        struct Configuration
        {
        private:
            std::string _fileName;
            std::vector<uint8_t> _validGlyphs;

            void _computeGlyphInformation(const std::vector<uint8_t> &p_fontData);

        public:
            Configuration();
            Configuration(const std::string &p_fileName, const std::vector<uint8_t> &p_fontData);

            const std::string &fileName() const;
            const std::vector<uint8_t>& validGlyphs() const;
        };

    public:
        /**
         * @class Key
         * @brief Represents a unique identifier for font atlas configurations.
         *
         * The Key class defines a set of parameters that uniquely identify a font atlas configuration,
         * including font size, outline size, and outline style. This enables the Font class to manage
         * multiple atlases for different text rendering settings efficiently. The Key class also includes
         * functionality to compute circle indices for pixelized and sharp edge outline styles, facilitating
         * advanced outline rendering techniques.
         *
         * Operators and methods within this class ensure that each Key is uniquely comparable, allowing
         * for their use as keys in a map for atlas caching and retrieval.
         *
         * Usage example: Not directly instantiated by users, used internally by Font to generate and cache
         * Atlases.
         *
         * @see Font, Atlas, OutlineStyle
         */
        struct Key
        {
            size_t fontSize;
            size_t outlineSize;
            size_t inverseOutlineSize;
            size_t outlineSizeSquared;
            OutlineStyle outlineStyle;
            std::vector<int> circleIndexes;

            Key(const size_t &p_fontSize, const size_t &p_outlineSize, const OutlineStyle &p_outlineStyle = OutlineStyle::Standard);
            bool operator<(const Key &p_other) const;
            void computeCircle(const spk::Vector2Int& p_atlasSize);
        };

        /**
         * @class Atlas
         * @brief Manages a texture atlas for a specific font configuration, including glyph data and texture.
         *
         * The Atlas class is a crucial component of the Font system, handling the creation and storage of a
         * texture atlas for a given font configuration. It includes glyph data for individual characters and
         * the texture itself, allowing for efficient text rendering.
         * The class provides methods to access glyph data, including texture coordinates and positioning,
         * tailored to the rendering requirements specified by a Font::Key.
         *
         * Atlases are generated dynamically based on font size, outline size, and style, optimizing texture
         * memory usage and rendering performance. The BuildData struct within Atlas facilitates the construction
         * of the atlas texture from raw font data.
         *
         * Usage example:
         * @code
         * spk::Font myFont("path/to/fontfile.ttf");
         * spk::Font::Atlas atlas = myFont.atlas(16, 1); // Get or create a font atlas for font size 16 with 1px outline
         * spk::Font::Atlas::GlyphData glyphE = atlas.glyph('E');
         * @endcode
         *
         * @see Font, Configuration, Key, GlyphData
         */
        class Atlas
        {
        public:
            /**
             * @class GlyphData
             * @brief Holds rendering data for an individual glyph within a font atlas.
             *
             * Nested within the Atlas class, GlyphData contains the texture coordinates (UVs), screen position express
             * in pixels, and step information express in pixel for a single glyph. This data is essential for rendering
             * the glyph on the screen, providing the necessary details to accurately place and scale the glyph as
             * part of text rendering operations.
             *
             * This struct is automatically populated when a Font::Atlas is created, ensuring that all glyphs within
             * the atlas are ready for efficient rendering.
             *
             * Usage example: Not directly instantiated by users, used internally by Atlas to store glyph information.
             *
             * @see Atlas
             */
            struct GlyphData
            {
                spk::Vector2 uvs[4];
                spk::Vector2Int position[4];
                spk::Vector2Int step;
            };

        private:
            struct BuildData
            {
                std::vector<uint8_t> buffer;
                spk::Vector2Int size = spk::Vector2Int(32, 32);
            };

            Configuration _fontConfiguration;
            std::vector<GlyphData> _glyphDatas;
            Texture _texture;

	        BuildData _computeBuildData(const std::vector<uint8_t> &p_fontData, const Configuration &p_fontConfiguration, const Key &p_key);

        public:
            Atlas();
            Atlas(const std::vector<uint8_t> &p_fontData, const Configuration &p_fontConfiguration, const Key &p_key);

            const GlyphData &glyph(const wchar_t &p_char) const;

            const GlyphData &operator [](const wchar_t &p_char) const;

            const Texture &texture() const;
        };

    private:
        std::vector<uint8_t> _fontData;
        Configuration _fontConfiguration;
        std::map<Key, Atlas*> _fontAtlas;

    public:
        Font(const std::filesystem::path &p_path);

        Atlas* atlas(const size_t &p_fontSize, const size_t &p_outlineSize) const;
        Atlas* atlas(const size_t &p_fontSize, const size_t &p_outlineSize);

        Atlas* atlas(const Key &p_fontAtlasKey) const;
        Atlas* atlas(const Key &p_fontAtlasKey);

        Atlas* operator[](const Key &p_fontAtlasKey) const;
        Atlas* operator[](const Key &p_fontAtlasKey);
    };
}