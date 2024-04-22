		
#include "graphics/texture/font/spk_font.hpp"
#include <array>

#include "unordered_map"

using OutlineMask = std::unordered_map<size_t, uint8_t>;

namespace spk
{
	OutlineMask _computeStandardOutlineMask(const size_t& p_outlineSize, const spk::Vector2Int& p_textureSize)
	{
		OutlineMask result;

		int outlineDistance = static_cast<int>(p_outlineSize * p_outlineSize);

        // The squared distance at which the fading starts (last two pixels)
        int fadeStartDistance = static_cast<int>((p_outlineSize - 2) * (p_outlineSize - 2));

        for (int x = -static_cast<int>(p_outlineSize); x <= static_cast<int>(p_outlineSize); x++)
        {
            for (int y = -static_cast<int>(p_outlineSize); y <= static_cast<int>(p_outlineSize); y++)
            {
                int distance = x * x + y * y;
                if (distance > outlineDistance)
                    continue;

                unsigned char value = 255; // Full intensity by default
                if (distance > fadeStartDistance)
                {
                    // Calculate the fading factor based on the distance
                    float fadeFactor = 1.0f - static_cast<float>(distance - fadeStartDistance) / ((p_outlineSize * p_outlineSize) - fadeStartDistance);
                    value = static_cast<unsigned char>(255 * fadeFactor);
                }

                result[x + y * p_textureSize.x] = value;
            }
        }

        return result;
	}

	OutlineMask _computeManhattanOutlineMask(const size_t& p_outlineSize, const spk::Vector2Int& p_textureSize)
	{
		OutlineMask result;

		int fadeStartDistance = p_outlineSize - 2;

		for (int x = -static_cast<int>(p_outlineSize); x <= static_cast<int>(p_outlineSize); x++)
		{
			for (int y = -static_cast<int>(p_outlineSize); y <= static_cast<int>(p_outlineSize); y++)
			{
				int manhattanDistance = std::abs(x) + std::abs(y);
				unsigned char value = 255; // Full intensity by default

				if (manhattanDistance > fadeStartDistance)
				{
					float fadeFactor = 1.0f - static_cast<float>(manhattanDistance - fadeStartDistance) / (p_outlineSize - fadeStartDistance);
					value = static_cast<unsigned char>(255 * fadeFactor);
				}

				if (manhattanDistance <= static_cast<int>(p_outlineSize))
				{
					result[x + y * p_textureSize.x] = value;
				}
			}
		}

		return result;
	}

	OutlineMask _computePixelisedOutlineMask(const size_t& p_outlineSize, const spk::Vector2Int& p_textureSize)
	{
		OutlineMask result;

		int outlineDistance = (p_outlineSize <= 3 ? 0 : static_cast<int>(p_outlineSize - 3));

		for (int x = -static_cast<int>(p_outlineSize); x <= static_cast<int>(p_outlineSize); x++)
		{
			unsigned char value = 255;
			int absX = std::abs(x);

			if (absX > outlineDistance)
			{
				float fadeFactor = 1.0f - (absX - outlineDistance) / 3.0f;
                value = static_cast<unsigned char>(255 * fadeFactor);
			}

			result[x] = value;
		}

		for (int y = -static_cast<int>(p_outlineSize); y <= static_cast<int>(p_outlineSize); y++)
		{
			unsigned char value = 255;
			int absY = std::abs(y);

			if (absY > outlineDistance)
			{
				float fadeFactor = 1.0f - (absY - outlineDistance) / 3.0f;
				value = static_cast<unsigned char>(255 * fadeFactor);
			}

			result[y * p_textureSize.x] = value;
		}

		return result;
	}

	OutlineMask _computeSharpEdgeOutlineMask(const size_t& p_outlineSize, const spk::Vector2Int& p_textureSize)
	{
		OutlineMask result;

		int fadeStartDistance = p_outlineSize - 2;

		for (int x = -static_cast<int>(p_outlineSize); x <= static_cast<int>(p_outlineSize); x++)
		{
			for (int y = -static_cast<int>(p_outlineSize); y <= static_cast<int>(p_outlineSize); y++)
			{
				int distance = std::max(std::abs(x), std::abs(y));
				unsigned char value = 255; // Full intensity by default

				if (distance > fadeStartDistance)
				{
					float fadeFactor = 1.0f - static_cast<float>(distance - fadeStartDistance) / 2;
					value = static_cast<unsigned char>(255 * fadeFactor);
				}

				if (distance <= static_cast<int>(p_outlineSize))
				{
					result[x + y * p_textureSize.x] = value;
				}
			}
		}

		return result;
	}

	OutlineMask computeOutlineMask(const spk::Font::OutlineStyle& p_style, const size_t& p_outlineSize, const spk::Vector2Int& p_textureSize)
	{
		switch (p_style)
		{
			case spk::Font::OutlineStyle::Standard:
				return (_computeStandardOutlineMask(p_outlineSize, p_textureSize));
			case spk::Font::OutlineStyle::Manhattan:
				return (_computeManhattanOutlineMask(p_outlineSize, p_textureSize));
			case spk::Font::OutlineStyle::SharpEdge:
				return (_computeSharpEdgeOutlineMask(p_outlineSize, p_textureSize));
			case spk::Font::OutlineStyle::Pixelized:
				return (_computePixelisedOutlineMask(p_outlineSize, p_textureSize));
			default :
				return (OutlineMask());
		}
	}

	std::vector<spk::Vector2Int> computeOutlineBorder(const std::vector<uint8_t>& p_buffer, const spk::Vector2Int& p_bufferSize)
	{
		std::vector<spk::Vector2Int> result;
		
		for (int i = 0; i < p_bufferSize.x; i++)
		{
			for (int j = 0; j < p_bufferSize.y; j++)
			{
				if (p_buffer[i + j * p_bufferSize.x] != 0)
				{
					bool isBorder = false;
					for (int y = -1; y <= 1 && !isBorder; y++)
					{
						for (int x = -1; x <= 1 && !isBorder; x++)
						{
							if (x == 0 && y == 0) continue;

							int checkX = i + x;
							int checkY = j + y;

							if (p_buffer[checkX + checkY * p_bufferSize.x] == 0)
							{
								isBorder = true;
								break;
							}
						}
					}

					if (isBorder)
					{
						result.emplace_back(i, j);
					}
				}
			}
		}

		return (result);
	}

	void Font::Atlas::_computeOutlineBuffer(BuildData& p_buildData, const Key &p_key)
	{
		p_buildData.outlineBuffer.resize(p_buildData.size.x * p_buildData.size.y, 0);

		if (p_key.outlineSize == 0 || p_key.outlineStyle == spk::Font::OutlineStyle::None)
		{
			return ;
		}

		OutlineMask outlineMask = computeOutlineMask(p_key.outlineStyle, p_key.outlineSize, p_buildData.size.x);

		std::vector<spk::Vector2Int> borderPixels = computeOutlineBorder(p_buildData.fontBuffer, p_buildData.size);

		for (const auto& borderPixel : borderPixels)
		{
			int index = borderPixel.x + borderPixel.y * p_buildData.size.x;
			for (const auto& [offset, value] : outlineMask)
			{
				int tmpIndex = index + offset;
				if (p_buildData.fontBuffer[tmpIndex] != CHAR_PIXEL)
				{
					p_buildData.outlineBuffer[tmpIndex] = std::max(p_buildData.outlineBuffer[tmpIndex], value);
				}
			}
		}
	}
}