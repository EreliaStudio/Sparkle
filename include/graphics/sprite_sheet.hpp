#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

#include "graphics/image.hpp"

namespace spk
{
	class SpriteSheet final : public Image
	{
	public:
		using Sprite = Texture::Section;

	private:
		Vector2UInt _nbSprite{0, 0};
		Vector2 _unit{0.0f, 0.0f};
		std::vector<Sprite> _sprites;

		SpriteSheet();

		void _buildSprites(const Vector2UInt &spriteCount);

	public:
		SpriteSheet(std::span<const std::uint8_t> data, const Vector2UInt &spriteCount);

		[[nodiscard]] static SpriteSheet open(const std::filesystem::path &path, const Vector2UInt &spriteCount);

		[[nodiscard]] const Vector2UInt &nbSprite() const noexcept;
		[[nodiscard]] const Vector2 &unit() const noexcept;
		[[nodiscard]] const std::vector<Sprite> &sprites() const noexcept;

		[[nodiscard]] std::size_t spriteID(const Vector2UInt &coord) const;
		[[nodiscard]] const Sprite &sprite(const Vector2UInt &coord) const;
		[[nodiscard]] const Sprite &sprite(std::size_t spriteID) const;
	};
}
