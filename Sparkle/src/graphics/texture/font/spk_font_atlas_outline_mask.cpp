		
#include "graphics/texture/font/spk_font.hpp"
#include <array>

#include "unordered_map"

using OutlineMask = std::unordered_map<size_t, uint8_t>;

namespace spk
{
	OutlineMask _computeStandardOutlineMask(const size_t& p_outlineSize, const spk::Vector2Int& p_textureSize)
	{
		OutlineMask result;

		int toCompare = p_outlineSize * p_outlineSize;

		for (int x = -static_cast<int>(p_outlineSize); x <= static_cast<int>(p_outlineSize); x++)
		{
			for (int y = -static_cast<int>(p_outlineSize); y <= static_cast<int>(p_outlineSize); y++)
			{
				int distance = static_cast<int>(x * x + y * y + 0.5f);
				if (distance > toCompare)
					continue;

				float powerFactor = 4.0f;
				float normalizedDistance = static_cast<float>(distance) / static_cast<float>(toCompare);
				float fadeFactor = 1 - std::pow(normalizedDistance, powerFactor);

				unsigned char value = static_cast<unsigned char>(255 * fadeFactor);
				result[x + y * p_textureSize.x] = value;
			}
		}

		return (result);
	}

	OutlineMask _computeManhattanOutlineMask(const size_t& p_outlineSize, const spk::Vector2Int& p_textureSize)
	{
		OutlineMask result;

		for (int x = -static_cast<int>(p_outlineSize); x <= static_cast<int>(p_outlineSize); x++)
		{
			for (int y = -static_cast<int>(p_outlineSize); y <= static_cast<int>(p_outlineSize); y++)
			{
				int manhattanDistance = std::abs(x) + std::abs(y);
				if (manhattanDistance <= static_cast<int>(p_outlineSize))
				{
					float powerFactor = 4.0f;
					float normalizedDistance = static_cast<float>(manhattanDistance) / static_cast<float>(p_outlineSize);
					float fadeFactor = 1 - std::pow(normalizedDistance, powerFactor);
					result[x + y * p_textureSize.x] = static_cast<unsigned char>(255 * fadeFactor);
				}
			}
		}

		return result;
	}

	OutlineMask _computePixelisedOutlineMask(const size_t& p_outlineSize, const spk::Vector2Int& p_textureSize)
	{
		OutlineMask result;

		for (int x = -static_cast<int>(p_outlineSize); x <= static_cast<int>(p_outlineSize); x++)
		{
			if (x != 0)
			{
				float powerFactor = 4.0f;
				float normalizedDistance = static_cast<float>(std::abs(x)) / static_cast<float>(p_outlineSize);
				float fadeFactor = 1 - std::pow(normalizedDistance, powerFactor);
				result[x] = static_cast<unsigned char>(255 * fadeFactor);
			}
		}

		for (int y = -static_cast<int>(p_outlineSize); y <= static_cast<int>(p_outlineSize); y++)
		{
			if (y != 0)
			{
				float powerFactor = 4.0f;
				float normalizedDistance = static_cast<float>(std::abs(y)) / static_cast<float>(p_outlineSize);
				float fadeFactor = 1 - std::pow(normalizedDistance, powerFactor);
				result[y * p_textureSize.x] = static_cast<unsigned char>(255 * fadeFactor);
			}
		}

		return result;
	}

	OutlineMask _computeSharpEdgeOutlineMask(const size_t& p_outlineSize, const spk::Vector2Int& p_textureSize)
	{
		OutlineMask result;

		for (int x = -static_cast<int>(p_outlineSize); x <= static_cast<int>(p_outlineSize); x++)
		{
			for (int y = -static_cast<int>(p_outlineSize); y <= static_cast<int>(p_outlineSize); y++)
			{
				int distance = std::max(std::abs(x), std::abs(y));
				if (distance <= static_cast<int>(p_outlineSize))
				{
					float powerFactor = 4.0f;
					float normalizedDistance = static_cast<float>(distance) / static_cast<float>(p_outlineSize);
					float fadeFactor = 1 - std::pow(normalizedDistance, powerFactor);
					result[x + y * p_textureSize.x] = static_cast<unsigned char>(255 * fadeFactor);
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