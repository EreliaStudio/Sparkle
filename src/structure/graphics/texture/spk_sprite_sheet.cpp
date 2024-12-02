#include "structure/graphics/texture/spk_sprite_sheet.hpp"

namespace spk
{
	SpriteSheet::SpriteSheet(const std::filesystem::path& p_path, const spk::Vector2UInt& p_spriteSize) :
		spk::Image(p_path)
	{
		if (p_spriteSize == Vector2UInt(0, 0))
		{
			throw std::invalid_argument("SpriteSheet can't be created with a null sprite size");
		}
		_nbSprite = p_spriteSize;
		_unit = Vector2(1.0f, 1.0f) / _nbSprite;

		if (p_spriteSize == Vector2UInt(1, 1))
		{
			_sprites.push_back({ Vector2(0.0f, 0.0f), _unit });
		}
		else
		{
			for (size_t y = 0; y < _nbSprite.y; y++)
			{
				for (size_t x = 0; x < _nbSprite.x; x++)
				{
					spk::Vector2 anchor = Vector2(static_cast<float>(x), static_cast<float>(y)) * _unit;
					_sprites.push_back({ anchor, _unit });
				}
			}
		}
	}

	const spk::Vector2UInt& SpriteSheet::nbSprite() const
	{
		return _nbSprite;
	}

	const spk::Vector2& SpriteSheet::unit() const
	{
		return _unit;
	}

	const std::vector<SpriteSheet::Sprite>& SpriteSheet::sprites() const
	{
		return _sprites;
	}

	size_t SpriteSheet::spriteID(const spk::Vector2UInt& p_spriteCoord) const
	{
		if (p_spriteCoord.x >= _nbSprite.x || p_spriteCoord.y >= _nbSprite.y)
			throw std::out_of_range("Sprite coordinates are out of range");
		return ((_nbSprite.x * p_spriteCoord.y) + p_spriteCoord.x);
	}

	const SpriteSheet::Sprite& SpriteSheet::sprite(const spk::Vector2UInt& p_spriteCoord) const
	{
		return sprite(spriteID(p_spriteCoord));
	}

	const SpriteSheet::Sprite& SpriteSheet::sprite(const size_t& p_spriteID) const
	{
		if (_sprites.size() <= p_spriteID)
			throw std::out_of_range("Sprite ID out of range");
		return _sprites[p_spriteID];
	}
}