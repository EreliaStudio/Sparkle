#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "design_pattern/contract_provider.hpp"
#include "stb_truetype.h"
#include "graphics/opengl/texture.hpp"
#include "math/vector2.hpp"

namespace spk
{
	class Font
	{
	public:
		using Data = std::vector<std::uint8_t>;
		using Codepoint = char32_t;
		using Text = std::u32string;

		struct Glyph
		{
			std::array<Vector2Int, 4> positions;
			std::array<Vector2, 4> uvs;
			Vector2Int step;
			Vector2Int baselineOffset;
			Vector2UInt size;

			static inline const std::array<unsigned int, 6> indexesOrder = {0, 1, 2, 2, 1, 3};

			void rescale(const Vector2 &scaleRatio);
		};

		struct Size
		{
			std::size_t glyph = 0;
			std::size_t outline = 0;

			constexpr Size() = default;
			constexpr Size(std::size_t glyph);
			constexpr Size(std::size_t glyph, std::size_t outline);

			[[nodiscard]] constexpr bool operator==(const Size &other) const noexcept;
			[[nodiscard]] constexpr bool operator!=(const Size &other) const noexcept;
			[[nodiscard]] constexpr bool operator<(const Size &other) const noexcept;
		};

	private:
		struct Resource
		{
			Data data;
			stbtt_fontinfo fontInfo{};

			explicit Resource(Data data);
		};

	public:
		class Atlas final : public Texture
		{
			friend class Font;

		public:
			using Contract = ContractProvider<>::Contract;
			using Job = ContractProvider<>::callback_type;

		private:
			enum class Quadrant
			{
				TopLeft,
				TopRight,
				DownLeft,
				DownRight
			};

			std::shared_ptr<Resource> _fontResource;
			ContractProvider<> _onEditionContractProvider;
			std::unordered_map<Codepoint, Glyph> _glyphs;
			Glyph _unknownGlyph;

			Quadrant _currentQuadrant = Quadrant::TopLeft;
			Vector2Int _quadrantAnchor{0, 0};
			Vector2UInt _quadrantSize{0, 0};
			Vector2Int _nextGlyphAnchor{0, 0};
			Vector2Int _nextLineAnchor{0, 0};
			std::size_t _textSize = 0;
			std::size_t _outlineSize = 0;

			Atlas(std::shared_ptr<Resource> fontResource, std::size_t textSize, std::size_t outlineSize);

			void _rescaleGlyphs(const Vector2 &scaleRatio);
			void _resizeData(const Vector2UInt &size);
			[[nodiscard]] Vector2Int _computeGlyphPosition(const Vector2UInt &glyphSize);
			void _applyGlyphPixel(const std::uint8_t *pixels, const Vector2Int &position, const Vector2UInt &size);
			void _loadGlyph(Codepoint codepoint);

		public:
			[[nodiscard]] Contract subscribe(const Job &job);

			void loadGlyphs(const Text &glyphsToLoad);
			void loadGlyphs(std::string_view utf8GlyphsToLoad);
			void loadAllRenderableGlyphs();

			[[nodiscard]] const Glyph &operator[](Codepoint codepoint);
			[[nodiscard]] const Glyph &glyph(Codepoint codepoint);

			[[nodiscard]] Vector2UInt computeCharSize(Codepoint codepoint);
			[[nodiscard]] Vector2UInt computeStringSize(const Text &string);
			[[nodiscard]] Vector2UInt computeStringSize(std::string_view utf8String);
			[[nodiscard]] Vector2Int computeStringBaselineOffset(const Text &string);
			[[nodiscard]] Vector2Int computeStringBaselineOffset(std::string_view utf8String);
		};

	private:
		std::map<Size, std::unique_ptr<Atlas>> _atlases;
		std::shared_ptr<Resource> _resource;

		void _loadFromFile(const std::filesystem::path &path);
		void _loadFromData(Data data);

	public:
		[[nodiscard]] static Text textFromUTF8(std::string_view text);
		[[nodiscard]] static Font fromRawData(Data data);

		Font() = default;
		explicit Font(const std::filesystem::path &path);

		[[nodiscard]] Vector2UInt computeCharSize(Codepoint codepoint, const Size &size);
		[[nodiscard]] Vector2UInt computeStringSize(const Text &string, const Size &size);
		[[nodiscard]] Vector2UInt computeStringSize(std::string_view utf8String, const Size &size);
		[[nodiscard]] Vector2Int computeStringBaselineOffset(const Text &string, const Size &size);
		[[nodiscard]] Vector2Int computeStringBaselineOffset(std::string_view utf8String, const Size &size);

		[[nodiscard]] Size computeOptimalTextSize(const Text &string, float outlineSizeRatio, const Vector2UInt &textArea);
		[[nodiscard]] Size computeOptimalTextSize(std::string_view utf8String, float outlineSizeRatio, const Vector2UInt &textArea);

		[[nodiscard]] Atlas &atlas(const Size &size);
	};

	constexpr Font::Size::Size(std::size_t glyph) :
		glyph(glyph)
	{
	}

	constexpr Font::Size::Size(std::size_t glyph, std::size_t outline) :
		glyph(glyph),
		outline(outline)
	{
	}

	constexpr bool Font::Size::operator==(const Size &other) const noexcept
	{
		return glyph == other.glyph && outline == other.outline;
	}

	constexpr bool Font::Size::operator!=(const Size &other) const noexcept
	{
		return (*this == other) == false;
	}

	constexpr bool Font::Size::operator<(const Size &other) const noexcept
	{
		return glyph < other.glyph || (glyph == other.glyph && outline < other.outline);
	}
}
