#pragma once

#include "structure/graphics/texture/spk_image.hpp"

#include "utils/spk_file_utils.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	class SpriteSheet : public Image
	{
	public:
		using Sprite = Image::Section;

	private:
		spk::Vector2UInt _nbSprite;
		spk::Vector2 _unit;
		std::vector<Sprite> _sprites;

		using Image::loadFromData;
		using Image::loadFromFile;

	public:
		static SpriteSheet fromRawData(
			const std::vector<uint8_t> &p_rawData,
			const spk::Vector2UInt &p_spriteSize,
			const Filtering &p_filtering = Filtering::Nearest,
			const Wrap &p_wrap = Wrap::ClampToEdge,
			const Mipmap &p_mipmap = Mipmap::Enable)
		{
			DEBUG_LINE();
			SpriteSheet result;

			DEBUG_LINE();
			result.loadFromData(p_rawData, p_spriteSize);
			result.setProperties(p_filtering, p_wrap, p_mipmap);

			DEBUG_LINE();
			return (result);
		}

		SpriteSheet()
		{
		}

		SpriteSheet(const std::filesystem::path &p_path, const spk::Vector2UInt &p_spriteSize) :
			spk::Image()
		{
			loadFromFile(p_path, p_spriteSize);
		}

		void loadFromFile(const std::filesystem::path &p_path, const spk::Vector2UInt &p_spriteSize)
		{
			loadFromData(spk::FileUtils::readFileAsBytes(p_path), p_spriteSize);
		}

		void loadFromData(const std::vector<uint8_t> &p_data, const spk::Vector2UInt &p_spriteSize)
		{
			Image::loadFromData(p_data);

			if (p_spriteSize == Vector2UInt(0, 0))
			{
				throw std::invalid_argument("SpriteSheet can't be created with a null sprite size");
			}
			_nbSprite = p_spriteSize;
			_unit = Vector2(1.0f, 1.0f) / _nbSprite;
			spk::Vector2 halfPixelSize = 0.5f / spk::Vector2(size().x, size().y);

			if (p_spriteSize == Vector2UInt(1, 1))
			{
				_sprites.push_back({Vector2(0.0f, 0.0f) + halfPixelSize, _unit - halfPixelSize});
			}
			else
			{
				for (size_t y = 0; y < _nbSprite.y; y++)
				{
					for (size_t x = 0; x < _nbSprite.x; x++)
					{
						spk::Vector2 anchor = Vector2(static_cast<float>(x), static_cast<float>(y)) * _unit;
						_sprites.push_back({anchor + halfPixelSize, _unit - halfPixelSize});
					}
				}
			}
		}

		const spk::Vector2UInt &nbSprite() const
		{
			return _nbSprite;
		}

		const spk::Vector2 &unit() const
		{
			return _unit;
		}

		const std::vector<Sprite> &sprites() const
		{
			return _sprites;
		}

		size_t spriteID(const spk::Vector2UInt &p_spriteCoord) const
		{
			if (p_spriteCoord.x >= _nbSprite.x || p_spriteCoord.y >= _nbSprite.y)
			{
				throw std::out_of_range("Sprite coordinates are out of range");
			}
			return ((_nbSprite.x * p_spriteCoord.y) + p_spriteCoord.x);
		}

		const Sprite &sprite(const spk::Vector2UInt &p_spriteCoord) const
		{
			return sprite(spriteID(p_spriteCoord));
		}

		const Sprite &sprite(const size_t &p_spriteID) const
		{
			if (_sprites.size() <= p_spriteID)
			{
				throw std::out_of_range("Sprite ID out of range");
			}
			return _sprites[p_spriteID];
		}
	};
}
