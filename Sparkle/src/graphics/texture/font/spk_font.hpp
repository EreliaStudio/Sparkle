#pragma once

#include <vector>
#include <map>
#include <filesystem>

#include "graphics/texture/spk_texture.hpp"

namespace spk
{
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

        class Atlas
        {
        public:
            struct GlyphData
            {
                spk::Vector2 uvs[4];
                spk::Vector2Int position[4];
                spk::Vector2Int step;
            };

            struct BuildData
            {
                std::vector<uint8_t> buffer;
                spk::Vector2Int size = spk::Vector2Int(32, 32);
            };

        private:
            Configuration _fontConfiguration;
            std::vector<GlyphData> _glyphDatas;
            Texture _texture;

	        BuildData _computeBuildData(const std::vector<uint8_t> &p_fontData, const Configuration &p_fontConfiguration, const Key &p_key);

        public:
            Atlas();
            Atlas(const std::vector<uint8_t> &p_fontData, const Configuration &p_fontConfiguration, const Key &p_key);

            const GlyphData &glyph(const char &p_char) const;

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