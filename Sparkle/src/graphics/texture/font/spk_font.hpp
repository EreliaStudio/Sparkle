#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "graphics/texture/spk_texture.hpp"
#include "miscellaneous/spk_position_alignment.hpp"

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
        static const uint8_t CHAR_PIXEL = 0xFF;  //!< The numerical representation of a char pixel inside the texture raw data.
        static const uint8_t EMPTY_PIXEL = 0x00; //!< The numerical representation of an empty pixel inside the texture raw data.

        /**
         * @enum OutlineStyle
         * @brief Defines the style of outlines applied to glyphs within a font atlas.
         *
         * This enumeration specifies the various outline styles that can be applied to glyphs when generating a font atlas. Each style affects the visual appearance of text rendered using the font atlas.
         *
         * - Pixelized: Applies a pixel-art style outline, giving glyphs a blocky appearance.
         * - SharpEdge: Applies a sharp, clear outline around glyphs for a defined look.
         * - Standard: Applies a standard outline, balancing clarity and smoothness.
         * - Manhattan: Uses the Manhattan distance metric for a unique outline effect.
         */
        enum class OutlineStyle
        {
            Pixelized,
            SharpEdge,
            Standard,
            Manhattan,
            None
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

            void _computeGlyphInformation(const std::vector<uint8_t>& p_fontData);

        public:
            /**
             * @brief Default constructor.
             *
             * This constructor initializes the configuration with an empty path and vector.
             */
            Configuration();

            /**
             * @brief Constructs a font configuration with the specified font file and font data.
             *
             * This constructor initializes the configuration with a path to the font file and a vector containing the font data. It also computes glyph information to filter out unsupported characters.
             *
             * @param p_fileName Path to the font file.
             * @param p_fontData Vector containing the font data.
             */
            Configuration(const std::string& p_fileName, const std::vector<uint8_t>& p_fontData);

            /**
             * @brief Gets the file name of the font.
             *
             * @return The file name as a string.
             */
            const std::string& fileName() const;

            /**
             * @brief Gets the list of valid glyphs.
             *
             * @return A vector of valid glyph identifiers.
             */
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

            size_t fontSize;                ///< The size of the font in points. This affects the overall scale of the glyphs within the atlas.
            OutlineStyle outlineStyle;      ///< The outline style needed for a specific atlas.
            size_t outlineSize;             ///< The thickness of the outline around the glyphs. A larger value creates a thicker outline.

            /**
             * @brief Constructs a Key with specified font size, outline size, and outline style.
             *
             * Initializes a Key object with the provided parameters, setting the basis for a font atlas configuration. This includes the size of the font, the thickness of the outline, and the style of the outline. Additional metrics such as inverse outline size and circle indexes may be computed as needed based on these parameters.
             *
             * @param p_fontSize The font size for the atlas configuration, affecting glyph scale.
             * @param p_outlineSize The thickness of the outline for glyphs in the atlas.
             * @param p_outlineStyle The style of the outline applied to glyphs. Defaults to Standard if not specified.
             */
            Key(const size_t& p_fontSize, const size_t& p_outlineSize, const OutlineStyle& p_outlineStyle = OutlineStyle::Standard);

            /**
             * @brief Comparison operator for Key objects.
             *
             * Defines a strict weak ordering of Key objects, enabling their use as keys in sorted containers such as std::map. This method compares Key objects based on their font size, outline size, and outline style in that order, ensuring that each Key is uniquely identifiable.
             *
             * @param p_other Another Key object to compare against.
             * @return True if this Key is considered less than `p_other`, false otherwise.
             */
            bool operator<(const Key& p_other) const;
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
             * @struct GlyphData
             * @brief Holds rendering data for an individual glyph within a font atlas.
             *
             * This struct encapsulates all necessary data for rendering a single glyph, including
             * texture coordinates (UVs), positions of glyph corners, and additional rendering metrics like step and size.
             * This data is crucial for precise placement and rendering of glyphs on the screen, ensuring text is displayed
             * accurately according to the font's design.
             *
             * Fields:
             * - uvs: Array of `spk::Vector2` representing the UV coordinates for texture mapping.
             * - position: Array of `spk::Vector2Int` specifying the screen positions of the glyph's corners, used in vertex specification.
             * - step: `spk::Vector2Int` used in advanced rendering calculations, such as kerning or glyph alignment.
             * - size: `spk::Vector2Int` representing the pixel dimensions of the glyph within the atlas.
             *
             * The structure also defines the order of vertex indices for rendering in OpenGL or similar APIs, ensuring
             * that glyphs are drawn correctly as quads using two triangles.
             */
            struct GlyphData
            {
                spk::Vector2 uvs[4];         ///< Texture coordinates (UV mapping) for the glyph.
                spk::Vector2Int position[4]; ///< Screen position of the glyph's corners, in pixels.
                spk::Vector2Int step;        ///< Step information for the glyph, used in rendering calculations.
                spk::Vector2Int size;        ///< Size of the glyph in pixels, representing width and height.

                /**
                 * @brief Static vector defining the order of vertex indices for rendering.
                 *
                 * This vector specifies the order in which vertices (defined by the `position` and `uvs` arrays)
                 * should be used to draw the glyph. The order is crucial for properly assembling the triangles that
                 * make up the glyph's quad in OpenGL or similar rendering APIs. The default order supports a common
                 * indexing pattern for rendering two triangles to form a rectangle.
                 */
                static inline std::vector<unsigned int> indexesOrder = {0, 1, 2, 2, 1, 3};
            };

        private:
            struct BuildData
            {
                std::vector<uint8_t> fontBuffer;
                std::vector<uint8_t> outlineBuffer;
                spk::Vector2Int size = spk::Vector2Int(32, 32);
                std::unordered_map<size_t, uint8_t> outlineMask;
            };

            Configuration _fontConfiguration;
            std::vector<GlyphData> _glyphDatas;

            Texture _texture;
            bool _needUploadToGPU;
            spk::Vector2Int _textureSize;
            std::vector<uint8_t> _texturePixelData;

            BuildData _computeBuildData(const std::vector<uint8_t>& p_fontData, const Configuration& p_fontConfiguration, const Key& p_key);

            void _computeFontBuffer(BuildData& p_buildData, const Key &p_key);
            
            void _computeOutlineBuffer(BuildData& p_buildData, const Key &p_key);
            void _pushCombinedTexture(BuildData& p_buildData);

        public:
            /**
             * @brief Default constructor for the Atlas class.
             *
             * Initializes an empty Atlas object. This constructor is typically used to create an Atlas instance before
             * explicitly setting its properties or generating its content through other methods. An Atlas created this way
             * will not contain any glyph data or texture information until such data is provided and processed.
             */
            Atlas();
            /**
             * @brief Constructs an Atlas with specified font data, font configuration, and key.
             *
             * Initializes an Atlas by generating a texture atlas based on the provided font data, configuration,
             * and key, which includes font size, outline size, and outline style.
             *
             * @param p_fontData Vector containing the font's raw data.
             * @param p_fontConfiguration Configuration for the font, including valid glyphs.
             * @param p_key Key determining the specific atlas configuration to generate.
             */
            Atlas(const std::vector<uint8_t>& p_fontData, const Configuration& p_fontConfiguration, const Key& p_key);

            /**
             * @brief Retrieves the GlyphData for a specified character.
             *
             * @param p_char The character to retrieve glyph data for.
             * @return A reference to the GlyphData struct containing rendering information for the specified character.
             */
            const GlyphData& operator[](const wchar_t& p_char) const;

            /**
             * @brief Gets the texture associated with this Atlas.
             *
             * Returns a reference to the Texture object representing the atlas texture. This texture contains all the
             * glyphs rendered according to the Atlas configuration and is used for drawing text on screen. Access to
             * this texture allows for direct use in rendering pipelines.
             *
             * @return A constant reference to the Texture object for this Atlas.
             */
            const Texture& texture() const;

            /**
             * @brief Checks if the atlas texture needs to be uploaded to the GPU.
             *
             * This method determines whether the atlas texture has been modified since the last upload to the GPU
             * and needs to be re-uploaded. This is typically used to optimize rendering performance by ensuring
             * that textures are only uploaded when necessary.
             *
             * @return True if the atlas texture needs to be uploaded to the GPU, false otherwise.
             */
            bool needUploadToGPU() const;

            /**
             * @brief Uploads the atlas texture to the GPU.
             *
             * If the atlas texture has been modified, this method uploads the texture to the GPU to make it
             * available for rendering. This method should be called after any changes to the texture data to ensure
             * that the updated texture is used in subsequent render operations.
             *
             * @note This method should only be called if needUploadToGPU() returns true to avoid unnecessary
             * GPU operations.
             */
            void uploadToGPU();
        };

    private:
        std::vector<uint8_t> _fontData;
        Configuration _fontConfiguration;
        mutable std::map<Key, Atlas> _fontAtlas;

        Atlas& operator[](const Key& p_fontAtlasKey) const;
        Atlas& operator[](const Key& p_fontAtlasKey);

    public:
        /**
         * @brief Constructs a Font object with the specified font file path.
         *
         * Initializes a Font instance by loading font data from a specified file path. This constructor is responsible for
         * reading the font file and preparing it for use in generating text rendering atlases. The font file should be a valid
         * font format that the system is capable of processing.
         *
         * @param p_path The filesystem path to the font file. This should be a path to a valid font file from which the font
         * data can be loaded.
         */
        Font(const std::filesystem::path& p_path);

        /**
         * @brief Retrieves or creates an Atlas with specified font size, outline size, and outline style.
         *
         * This method generates a texture atlas for the specified font size, outline size, and outline style if it does not
         * already exist. If an atlas with the given parameters already exists, it returns a const reference to the existing atlas.
         * This allows for efficient text rendering by reusing atlases for common configurations.
         *
         * The method ensures that text rendering is optimized by dynamically generating atlases based on the specific
         * requirements of the text to be rendered, such as font size and outline style.
         *
         * @param p_fontSize The desired font size for the text rendering. This affects the scale of the glyphs within the atlas.
         * @param p_outlineSize The thickness of the glyph outlines in the atlas. This parameter allows for customization of the text's appearance.
         * @param p_outlineStype The style of the outline applied to the glyphs. This allows for further customization of how text is rendered.
         * @return A constant reference to the Atlas object configured with the specified parameters.
         */
        const Atlas& atlas(const size_t& p_fontSize, const size_t& p_outlineSize, const OutlineStyle& p_outlineStype) const;

        /**
         * @brief Retrieves or creates an Atlas with specified font size, outline size, and outline style.
         *
         * This method generates a texture atlas for the specified font size, outline size, and outline style if it does not
         * already exist. If an atlas with the given parameters already exists, it returns a reference to the existing atlas.
         * This allows for efficient text rendering by reusing atlases for common configurations.
         *
         * The method ensures that text rendering is optimized by dynamically generating atlases based on the specific
         * requirements of the text to be rendered, such as font size and outline style.
         *
         * @param p_fontSize The desired font size for the text rendering. This affects the scale of the glyphs within the atlas.
         * @param p_outlineSize The thickness of the glyph outlines in the atlas. This parameter allows for customization of the text's appearance.
         * @param p_outlineStype The style of the outline applied to the glyphs. This allows for further customization of how text is rendered.
         * @return A reference to the Atlas object configured with the specified parameters.
         */
        Atlas& atlas(const size_t& p_fontSize, const size_t& p_outlineSize, const OutlineStyle& p_outlineStype);
    };
}