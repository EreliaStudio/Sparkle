#include "graphics/sprite_sheet.hpp"

#include <stdexcept>

namespace spk
{
	SpriteSheet::SpriteSheet() :
		Image()
	{
	}

	SpriteSheet::SpriteSheet(std::span<const std::uint8_t> data, const Vector2UInt &spriteCount) :
		Image(data)
	{
		_buildSprites(spriteCount);
	}

	void SpriteSheet::_buildSprites(const Vector2UInt &spriteCount)
	{
		if (spriteCount.x == 0 || spriteCount.y == 0)
		{
			throw std::invalid_argument("SpriteSheet: sprite count cannot be zero");
		}

		_nbSprite = spriteCount;
		_unit = Vector2(1.0f, 1.0f) / Vector2(static_cast<float>(_nbSprite.x), static_cast<float>(_nbSprite.y));
		_sprites.clear();
		_sprites.reserve(static_cast<std::size_t>(_nbSprite.x) * static_cast<std::size_t>(_nbSprite.y));

		for (std::size_t y = 0; y < _nbSprite.y; ++y)
		{
			for (std::size_t x = 0; x < _nbSprite.x; ++x)
			{
				const Vector2 anchor(static_cast<float>(x) * _unit.x, static_cast<float>(y) * _unit.y);
				_sprites.emplace_back(anchor, _unit);
			}
		}
	}

	SpriteSheet SpriteSheet::open(const std::filesystem::path &path, const Vector2UInt &spriteCount)
	{
		SpriteSheet result;
		result._loadFromFile(path);
		result._buildSprites(spriteCount);
		return result;
	}

	const Vector2UInt &SpriteSheet::nbSprite() const noexcept
	{
		return _nbSprite;
	}

	const Vector2 &SpriteSheet::unit() const noexcept
	{
		return _unit;
	}

	const std::vector<SpriteSheet::Sprite> &SpriteSheet::sprites() const noexcept
	{
		return _sprites;
	}

	std::size_t SpriteSheet::spriteID(const Vector2UInt &coord) const
	{
		if (coord.x >= _nbSprite.x || coord.y >= _nbSprite.y)
		{
			throw std::out_of_range("SpriteSheet: sprite coordinates out of range");
		}

		return static_cast<std::size_t>(_nbSprite.x) * static_cast<std::size_t>(coord.y) + static_cast<std::size_t>(coord.x);
	}

	const SpriteSheet::Sprite &SpriteSheet::sprite(const Vector2UInt &coord) const
	{
		return sprite(spriteID(coord));
	}

	const SpriteSheet::Sprite &SpriteSheet::sprite(std::size_t spriteID) const
	{
		if (spriteID >= _sprites.size())
		{
			throw std::out_of_range("SpriteSheet: sprite ID out of range");
		}

		return _sprites[spriteID];
	}
}
