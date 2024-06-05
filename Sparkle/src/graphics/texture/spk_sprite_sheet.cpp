#include "graphics/texture/spk_sprite_sheet.hpp"

namespace spk
{
	SpriteSheet::SpriteSheet(const std::filesystem::path& p_path, const spk::Vector2UInt& p_spriteSize) :
		spk::Image(p_path)
	{
		_nbSprite = p_spriteSize;
		_unit = Vector2(1.0f, 1.0f) / _nbSprite;

		if (p_spriteSize == Vector2UInt(1, 1))
			_sprites.push_back(Vector2(0, 0));
		else
		{
			for (size_t y = 0; y < _nbSprite.y; y++)
			{
				for (size_t x = 0; x < _nbSprite.x; x++)
				{
					_sprites.push_back(Vector2(static_cast<float>(x), static_cast<float>(y)) * _unit);
				}
			}
		}
	}

	const spk::Vector2UInt& SpriteSheet::nbSprite() const
	{
		return (_nbSprite);
	}

	const spk::Vector2& SpriteSheet::unit() const
	{
		return (_unit);
	}

	const std::vector<spk::Vector2>& SpriteSheet::sprites() const
	{
		return (_sprites);
	}

	size_t SpriteSheet::spriteID(const spk::Vector2UInt& p_spriteCoord) const
	{	
		return ((_nbSprite.x * p_spriteCoord.y) + p_spriteCoord.x);
	}

	spk::Vector2 SpriteSheet::sprite(const spk::Vector2UInt& p_spriteCoord) const
	{
		return (sprite(spriteID(p_spriteCoord)));
	}
	
	spk::Vector2 SpriteSheet::sprite(const size_t& p_spriteID) const
	{
		if (_sprites.size() <= p_spriteID)
			throwException("Segfault : sprite id out of range");
		return (_sprites[p_spriteID]);
	}
}